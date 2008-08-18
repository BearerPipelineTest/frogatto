#include <iostream>

#include "character.hpp"
#include "font.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "joystick.hpp"
#include "level.hpp"
#include "level_logic.hpp"
#include "raster.hpp"
#include "sound.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

character::character(wml::const_node_ptr node)
  : entity(node),
    type_(character_type::get(node->attr("type"))),
	previous_y_(y()),
	velocity_x_(wml::get_int(node, "velocity_x")),
	velocity_y_(wml::get_int(node, "velocity_y")),
	invincible_(0),
	lvl_(NULL),
	walk_formula_(game_logic::formula::create_optional_formula(node->attr("walk_formula"))),
	jump_formula_(game_logic::formula::create_optional_formula(node->attr("jump_formula"))),
	fly_formula_(game_logic::formula::create_optional_formula(node->attr("fly_formula"))),
	xpos_formula_(game_logic::formula::create_optional_formula(node->attr("xpos_formula"))),
	ypos_formula_(game_logic::formula::create_optional_formula(node->attr("ypos_formula"))),
	formula_test_frequency_(wml::get_int(node, "formula_test_frequency", 10)),
	time_since_last_formula_(0),
	walk_result_(0), jump_result_(0), fly_result_(0),
	collided_since_last_frame_(false),
	time_in_frame_(0),
	hitpoints_(wml::get_int(node, "hitpoints", type_->hitpoints())),
	max_hitpoints_(wml::get_int(node, "max_hitpoints", type_->hitpoints())),
	walk_speed_(wml::get_int(node, "walk_speed", type_->walk())),
	jump_power_(wml::get_int(node, "jump_power", type_->jump())),
	boost_power_(wml::get_int(node, "boost_power", type_->boost())),
	glide_speed_(wml::get_int(node, "glide_speed", type_->glide())),
	cycle_num_(0), last_jump_(false), frame_id_(0)
{
	current_frame_ = &type_->get_frame();
	assert(type_);
}

character::character(const std::string& type, int x, int y, bool face_right)
  : entity(x, y, face_right),
    type_(character_type::get(type)),
	previous_y_(y),
	velocity_x_(0),
	velocity_y_(0),
	invincible_(0),
	lvl_(NULL),
	formula_test_frequency_(1),
	time_since_last_formula_(0),
	walk_result_(0), jump_result_(0), fly_result_(0),
	collided_since_last_frame_(false),
	time_in_frame_(0),
	hitpoints_(type_->hitpoints()),
	max_hitpoints_(type_->hitpoints()),
	walk_speed_(type_->walk()),
	jump_power_(type_->jump()),
	boost_power_(type_->boost()),
	glide_speed_(type_->glide()),
	cycle_num_(0), last_jump_(false), frame_id_(0)
{
	current_frame_ = &type_->get_frame();
	assert(type_);
}

void character::set_level(level* lvl)
{
	lvl_ = lvl;
}

wml::node_ptr character::write() const
{
	wml::node_ptr res(new wml::node("character"));
	res->set_attr("type", type_->id());
	res->set_attr("face_right", face_right() ? "true" : "false");
	res->set_attr("x", formatter() << x());
	res->set_attr("y", formatter() << y());
	res->set_attr("velocity_x", formatter() << velocity_x_);
	res->set_attr("velocity_y", formatter() << velocity_y_);
	res->set_attr("formula_test_frequency", formatter() << formula_test_frequency());
	if(hitpoints_ != type_->hitpoints()) {
		res->set_attr("hitpoints", formatter() << hitpoints_);
	}
	if(max_hitpoints_ != type_->hitpoints()) {
		res->set_attr("max_hitpoints", formatter() << max_hitpoints_);
	}
	if(walk_speed_ != type_->walk()) {
		res->set_attr("walk_speed", formatter() << walk_speed_);
	}
	if(jump_power_ != type_->jump()) {
		res->set_attr("jump_power", formatter() << jump_power_);
	}
	if(boost_power_ != type_->boost()) {
		res->set_attr("boost_power", formatter() << boost_power_);
	}
	if(glide_speed_ != type_->glide()) {
		res->set_attr("glide_speed", formatter() << glide_speed_);
	}
	if(walk_formula_) {
		res->set_attr("walk_formula", walk_formula_->str());
	}
	if(jump_formula_) {
		res->set_attr("jump_formula", jump_formula_->str());
	}
	if(fly_formula_) {
		res->set_attr("fly_formula", fly_formula_->str());
	}
	if(xpos_formula_) {
		res->set_attr("xpos_formula", xpos_formula_->str());
	}
	if(ypos_formula_) {
		res->set_attr("ypos_formula", ypos_formula_->str());
	}
	if(is_human()) {
		res->set_attr("is_human", "true");
	}

	if(group() >= 0) {
		res->set_attr("group", formatter() << group());
	}
	return res;
}

void character::draw() const
{
	if(driver_) {
		driver_->draw();
	}
	const int slope = current_frame().rotate_on_slope() ? -slope_standing_on(5)*face_dir() : 0;
	current_frame().draw(x(), y(), face_right(), time_in_frame_, slope);

	//if we blur then back up information about the frame here
	if(current_frame().blur()) {
		blur_.push_back(previous_draw());
		previous_draw& p = blur_.back();
		p.frame_drawn = &current_frame();
		p.x = x();
		p.y = y();
		p.face_right = face_right();
		p.time_in_frame = time_in_frame_;
		p.alpha = 100;
		p.blur = current_frame().blur();
		p.slope = slope;
	}

	//draw any blurred frames
	std::vector<previous_draw>::iterator p = blur_.begin();
	while(p != blur_.end()) {
		std::cerr << "draw blurred frame\n";
		p->alpha = (p->alpha*p->blur)/100;
		glColor4f(1.0, 1.0, 1.0, p->alpha/100.0);
		p->frame_drawn->draw(p->x, p->y, p->face_right, p->time_in_frame, p->slope);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		if(p->alpha < 5) {
			p = blur_.erase(p);
		} else {
			++p;
		}
	}
}

void character::draw_group() const
{
	if(group() >= 0) {
		blit_texture(font::render_text(formatter() << group(), graphics::color_yellow(), 24), x(), y());
	}
}

void pc_character::draw() const
{
	if(is_human() && ((invincible()/5)%2) == 1 && &current_frame() != type().gethit_frame()) {
		return;
	}
	
	character::draw();
}

void character::process(level& lvl)
{
	set_level(&lvl);
	if(y() > lvl.boundaries().y2()) {
		--hitpoints_;
	}

	previous_y_ = y();

	++cycle_num_;
	const int start_x = x();
	const int start_y = y();

	++time_in_frame_;

	//check if we're half way through our crouch in which case we lock
	//the frame in place until uncrouch() is called.
	if(time_in_frame_ == current_frame_->duration()/2 &&
	   (current_frame_ == type_->crouch_frame() ||
	    current_frame_ == type_->lookup_frame())) {
	   --time_in_frame_;
	}

	if(time_in_frame_ == current_frame_->duration() && current_frame_ != type_->jump_frame() && current_frame_ != type_->fall_frame() && current_frame_ != type_->gethit_frame() && current_frame_ != type_->die_frame()) {
		if(current_frame_ == &type_->get_frame()) {
			change_frame((rand()%5) == 0 ? type_->idle_frame() : &type_->get_frame());
		} else if(current_frame_ == type_->stand_up_slope_frame()) {
			time_in_frame_ = 0;
		} else if(current_frame_ == type_->stand_down_slope_frame()) {
			time_in_frame_ = 0;
		} else if(current_frame_ == type_->idle_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->walk_frame()) {
			time_in_frame_ = 0;
			if(velocity_x_/100 == 0) {
				change_to_stand_frame();
			}
		} else if(current_frame_ == type_->fly_frame()) {
			time_in_frame_ = 0;
		} else if(current_frame_ == type_->crouch_frame() ||
		          current_frame_ == type_->lookup_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->attack_frame() ||
		          current_frame_ == type_->up_attack_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->run_attack_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->slide_frame()) {
			change_frame(type_->fall_frame());
		} else if(current_frame_ == type_->jump_attack_frame()) {
			change_frame(type_->fall_frame());
		} else if(current_frame_ == type_->spring_frame()) {
			time_in_frame_ = 0;
			change_to_stand_frame();
		} else if(current_frame_ == type_->turn_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->run_frame()) {
			time_in_frame_ = 0;
			current_frame_ = type_->run_frame();
		}
	}

	if((current_frame_ == type_->walk_frame() || current_frame_ == type_->run_frame()) && velocity_x_/100 == 0) {
		change_to_stand_frame();
	}
	
	control(lvl);

	collided_since_last_frame_ = false;

	int adjust_y_pos = 0;
	const bool started_standing = is_standing(lvl, NULL, NULL, &adjust_y_pos);
	set_pos(x(), y() + adjust_y_pos);

	if(invincible_) {
		--invincible_;
	}

	int climb = type_->climb();
	for(int n = 0; n != std::abs(velocity_x_/100); ++n) {
		const int dir = velocity_x_/100 > 0 ? 1 : -1;
		int xpos = (dir < 0 ? body_rect().x() : body_rect().x2() - 1) + dir;

		bool collide = false, hit = false;
		const int ybegin = y() + current_frame().collide_y();
		const int yend = y() + current_frame().collide_y() + current_frame().collide_h();
		for(int ypos = ybegin; ypos != yend; ++ypos) {
			int damage = 0;
			if(lvl.solid(xpos, ypos, NULL, invincible_ ? NULL : &damage)) {
				if(!damage) {
					collide = true;
					break;
				} else {
					hit = true;
					break;
				}
			}
		}

		if(!hit) {
			entity_ptr collide_with = lvl.collide(rect(xpos, ybegin, 1, yend - ybegin), this);
			if(collide_with) {
				if(collide_with->body_harmful() && is_human()) {
					if(!invincible_) {
						hit = true;
					}
				} else {
					collide = true;
				}
			}
		}

		if(hit) {
			velocity_x_ = -dir*200;
			velocity_y_ = -400;
			hit = true;
			get_hit();
			break;
		}

		if(hit) {
			break;
		}

		if(collide) {
			if((current_frame_ == type_->fall_frame() || current_frame_ == type_->jump_frame()) && type_->slide_frame() && velocity_y_ <= 100 && (velocity_x_ > 0) == face_right() && std::abs(velocity_x_) > 300) {
				change_frame(type_->slide_frame());
				velocity_y_ = 0;
			}
			velocity_x_ = 0;
			collided_since_last_frame_ = true;
			break;
		}

		set_pos(x() + dir, y());

		int adjust_y = 0;
		set_pos(x(), y()-1);
		while(is_standing(lvl, NULL, NULL, &adjust_y)) {
			set_pos(x(), y()-1);
		}

		set_pos(x(), y()+1);

		set_pos(x(), y() + adjust_y);

		if(started_standing) {
			try_to_make_standing();
		}
	}

	for(int n = 0; n < velocity_y_/100; ++n) {
		if(is_standing(lvl)) {
			velocity_y_ = 0;
			break;
		}

		//bounce off someone's head
		entity_ptr c = lvl.collide(feet_x() - FeetWidth, feet_y(), this);
		if(!c) {
			c = lvl.collide(feet_x() + FeetWidth, feet_y(), this);
		}

		if(c) {
			if(c->springiness() > 0) {
				velocity_y_ = -c->springiness()*13;
				if(c->velocity_y() < 0) {
					velocity_y_ += c->velocity_y();
				}
			}

			//make sure we don't end up bouncing over and over without control
			//while getting hit by making bouncing off someone's head turn us
			//into the jump frame, which we have control over.
			if(current_frame_ == type_->gethit_frame()) {
				change_frame(type_->jump_frame());
			}

			c->spring_off_head(*this);
		}

		//see if we're boarding a vehicle
		if(!type_->is_vehicle()) {
			c = lvl.board(feet_x(), feet_y());
			if(c) {
				c->boarded(lvl, this);
				return;
			}
		}

		set_pos(x(), y() + 1);
	}

	for(int n = 0; n < -velocity_y_/100; ++n) {
		const int ypos = y() + current_frame().collide_y() - 1;
		bool collide = false, hit = false;
		const int left = collide_left();
		const int right = collide_right();
		for(int xpos = left; xpos < right; ++xpos) {
			int damage = 0;
			if(lvl.solid(xpos, ypos, NULL, invincible_ ? NULL : &damage)) {
				if(!damage) {
					collide = true;
					break;
				} else {
					hit = true;
				}
			}
		}

		if(!hit) {
			entity_ptr collide_with = lvl.collide(body_rect(), this);
			if(collide_with) {
				if(collide_with->body_harmful() && is_human()) {
					if(!invincible_) {
						hit = true;
					}
				} else {
					collide = true;
				}
			}
		}

		if(hit) {
			hit = true;
			get_hit();
			break;
		}

		if(!is_human() && !invincible_) {
			character_ptr player = lvl.hit_by_player(body_rect());
			if(player) {
				set_face_right(!player->face_right());
				hit = true;
				get_hit();
				break;
			}
		}

		if(hit) {
			break;
		}

		if(collide) {
			collided_since_last_frame_ = true;
			velocity_y_ = 0;
			break;
		}

		set_pos(x(), y() - 1);
	}

	//adjust position if colliding with terrain
	{
		const int dir = face_right() ? 1 : -1;
		int x1 = current_frame().collide_x();
		if(dir > 0) {
			x1 += x();
		} else {
			x1 = x() + current_frame().width() - x1;
		}

		int x2 = x1 + current_frame().collide_w()*dir;
		if(x1 > x2) {
			std::swap(x1, x2);
		}

		const int ypos = y() + current_frame().collide_y() + current_frame().collide_h();
		const bool standing = is_standing(*lvl_);
		if(lvl.solid(x1,ypos) || lvl.collide(x1, ypos)) {
			std::cerr << "PUSH_BACK!\n";
			set_pos(x() + 1, y());
			if(started_standing) {
				try_to_make_standing();
			}
		}

		if(lvl.solid(x2,ypos) || lvl.collide(x2, ypos)) {
			std::cerr << "PUSH_BACK!\n";
			set_pos(x() - 1, y());
			if(started_standing) {
				try_to_make_standing();
			}
		}

		if(standing && !is_standing(*lvl_)) {
			std::cerr << "NOT STANDING!!!\n";
		}
	}

	if(is_human() && !invincible_) {
		entity_ptr collide_with = lvl_->collide(body_rect(), this);
		if(collide_with && collide_with->body_harmful()) {
			get_hit();
		}
	}

	if(!is_human() && !invincible_) {
		character_ptr player = lvl.hit_by_player(body_rect());
		if(player) {
			set_face_right(!player->face_right());
			get_hit();
		}
	}

	int accel_x = current_frame().accel_x();
	if(!face_right()) {
		accel_x *= -1;
	}

	velocity_x_ += accel_x;

	const int AirResistance = 5;
	int friction = 0;
	int damage = 0;
	entity_ptr standing_on;
	const bool standing = is_standing(lvl, &friction, &damage, NULL, &standing_on);
	friction += lvl.air_resistance();
	friction = (friction * type_->traction())/100;
	velocity_x_ = (velocity_x_*(100-friction))/100;
	if(damage && !invincible()) {
		get_hit();
	} else if(standing && velocity_y_ >= 0) {
		if(standing_on) {
			standing_on->stood_on_by(this);
		}

		if(current_frame_ == type_->jump_frame() || current_frame_ == type_->fall_frame() || current_frame_ == type_->gethit_frame() || current_frame_ == type_->slide_frame() || current_frame_ == type_->jump_attack_frame()) {
			change_to_stand_frame();
		}
	} else if(in_stand_frame() || current_frame_ == type_->walk_frame() ||
	          current_frame_ == type_->run_frame() ||
			  current_frame_ == type_->idle_frame() ||
			  velocity_y_ + current_frame().accel_y() > 0 &&
			  current_frame_ != type_->jump_attack_frame() &&
			  current_frame_ != type_->fall_frame() &&
			  current_frame_ != type_->gethit_frame() &&
			  current_frame_ != type_->fly_frame() &&
			  (current_frame_ != type_->slide_frame() ||
			   !can_continue_sliding())) {
		if(type_->fall_frame()) {
			change_frame(type_->fall_frame());
		}
	}

	velocity_y_ += current_frame().accel_y();

	const int dx = x() - start_x;
	const int dy = y() - start_y;
	if(dx || dy) {
		foreach(const entity_ptr& ch, standing_on_) {
			ch->set_pos(point(ch->x() + dx, ch->y() + dy));
		}
	}

	standing_on_.clear();

	set_driver_position();
}

void character::set_driver_position()
{
	if(driver_) {
		const int pos_right = x() + type_->passenger_x();
		const int pos_left = x() + current_frame().width() - driver_->current_frame().width() - type_->passenger_x();
		driver_->set_face_right(face_right());

		if(current_frame_ == type_->turn_frame()) {
			int weight_left = time_in_frame_;
			int weight_right = current_frame_->duration() - time_in_frame_;
			if(face_right()) {
				std::swap(weight_left, weight_right);
			}
			const int pos = (pos_right*weight_right + pos_left*weight_left)/current_frame_->duration();
			driver_->set_pos(pos, y() + type_->passenger_y());
		} else {
			driver_->set_pos(face_right() ? pos_right : pos_left, y() + type_->passenger_y());
		}
	}
}

void character::try_to_make_standing()
{
	const int MaxStep = 3;
	int max_down = MaxStep;

	while(!is_standing(*lvl_) && --max_down) {
		set_pos(x(), y()+1);
	}

	if(!is_standing(*lvl_)) {
		set_pos(x(), y() - MaxStep);

		int max_up = MaxStep;
		while(!is_standing(*lvl_) && --max_up) {
			set_pos(x(), y()-1);
		}

		if(!is_standing(*lvl_)) {
			set_pos(x(), y() + MaxStep);
		}
	}
}

bool character::is_standing(const level& lvl, int* friction, int* damage, int* adjust_y, entity_ptr* standing_on) const
{
	return lvl.standable(feet_x()-FeetWidth, feet_y(), friction, damage, adjust_y, standing_on, this) ||
	       lvl.standable(feet_x()+FeetWidth, feet_y(), friction, damage, adjust_y, standing_on, this);
}

bool character::destroyed() const
{
	return hitpoints_ <= 0 && (current_frame_ != type_->die_frame() || time_in_frame_ >= current_frame_->duration());
}

void character::set_face_right(bool facing)
{
	if(facing == face_right()) {
		return;
	}

	if(is_standing(*lvl_) || current_frame_ == type_->fly_frame()) {
		change_frame(type_->turn_frame());
	}

	const int original_pos = feet_x();
	entity::set_face_right(facing);
	const int diff_x = feet_x() - original_pos;
	set_pos(x() - diff_x, y());
}

int character::springiness() const
{
	return type_->springiness();
}

void character::spring_off_head(const entity& jumped_on_by)
{
	std::cerr << "sprung off head!\n";
	if(type_->spring_frame()) {
		change_frame(type_->spring_frame());
		std::cerr << "set to spring frame\n";
	}
}

void character::boarded(level& lvl, character_ptr player)
{
	player->current_frame_ = &player->type_->get_frame();
	character_ptr new_player(new pc_character(*this));
	new_player->driver_ = player;
	lvl.add_player(new_player);
	hitpoints_ = 0;
}

void character::unboarded(level& lvl)
{
	character_ptr vehicle(new character(*this));
	vehicle->driver_ = character_ptr();
	lvl.add_character(vehicle);
	lvl.add_player(driver_);
	driver_->set_velocity(600 * (face_right() ? 1 : -1), -600);
}

int character::collide_left() const
{
	if(face_right()) {
		return x() + current_frame().collide_x();
	} else {
		return x() + current_frame().width() - current_frame().collide_x() - current_frame().collide_w();
	}
}

int character::collide_right() const
{
	if(face_right()) {
		return x() + current_frame().collide_x() + current_frame().collide_w();
	} else {
		return x() + current_frame().width() - current_frame().collide_x();
	}
}

void character::walk(const level& lvl, bool move_right)
{
	if(current_frame_ == type_->slide_frame() || current_frame_ == type_->spring_frame() || current_frame_ == type_->die_frame() || current_frame_ == type_->turn_frame() || current_frame_ == type_->gethit_frame()) {
		return;
	}

	const bool standing = is_standing(lvl);
	set_face_right(move_right);
	const int run_bonus = current_frame_ == type_->run_frame() ? 2 : 1;
	velocity_x_ += (standing ? walk_speed()*run_bonus : glide_speed())*(move_right ? 1 : -1);
	if(standing && current_frame_ != type_->walk_frame() && type_->walk_frame() && current_frame_ != type_->jump_frame() && current_frame_ != type_->turn_frame() && current_frame_ != type_->run_frame()) {
		change_frame(type_->walk_frame());
	}

}

void character::run(const level& lvl, bool move_right)
{
	if(current_frame_ == type_->walk_frame()) {
		change_frame(type_->run_frame());
	}
}

void character::fly(const level& lvl, bool move_right, int lift)
{
	set_face_right(move_right);
	velocity_x_ += glide_speed()*(move_right ? 1 : -1);
	velocity_y_ += lift;

	if(current_frame_ != type_->fly_frame() && current_frame_ != type_->turn_frame() && current_frame_ != type_->spring_frame()) {
		change_frame(type_->fly_frame());
	}
}

void character::jump(const level& lvl)
{
	entity_ptr platform;
	if(!last_jump_ && current_frame_ == type_->slide_frame()) {
		set_face_right(!face_right());
		velocity_x_ += glide_speed()*(face_right() ? 15 : -15);
		velocity_y_ = (-jump_power()*3)/4;
		if(type_->jump_frame()) {
			change_frame(type_->jump_frame());
		}
	} else if(!last_jump_ && is_standing(lvl, NULL, NULL, NULL, &platform)) {
		if(platform) {
			velocity_x_ += platform->velocity_x();
			velocity_y_ += platform->velocity_y();
		}
		velocity_y_ = -jump_power();
		if(type_->jump_frame()) {
			change_frame(type_->jump_frame());
		}
	} else if(velocity_y_ < 0) {
		velocity_y_ -= boost_power();
	}
}

void character::jump_down(const level& lvl)
{
	if(driver_) {
		unboarded(*lvl_);
	}

	if(is_standing(lvl)) {
		set_pos(x(), y() + 1);
		if(is_standing(lvl)) {
			set_pos(x(), y() - 1);
		}
	}
}

void character::crouch(const level& lvl)
{
	if(is_standing(lvl) && current_frame_ != type_->crouch_frame()) {
		change_frame(type_->crouch_frame());
	}
}

void character::uncrouch(const level& lvl)
{
	if(time_in_frame_ == current_frame_->duration()/2 - 1) {
		++time_in_frame_;
	}
}

void character::lookup(const level& lvl)
{
	if(is_standing(lvl) && current_frame_ != type_->lookup_frame()) {
		change_frame(type_->lookup_frame());
	}
}

void character::unlookup(const level& lvl)
{
	if(time_in_frame_ == current_frame_->duration()/2 - 1) {
		++time_in_frame_;
	}
}

void character::attack(const level& lvl)
{
	if(is_standing(lvl)) {
		if(type_->run_attack_frame() && current_frame_ == type_->run_frame()) {
			change_frame(type_->run_attack_frame());
		} else if(type_->up_attack_frame() && look_up()) {
			change_frame(type_->up_attack_frame());
		} else {
			change_frame(type_->attack_frame());
		}
	} else if(current_frame_ == type_->jump_frame() || current_frame_ == type_->fall_frame()) {
		change_frame(type_->jump_attack_frame());
	}
}

const frame& character::current_frame() const
{
	return *current_frame_;
}

bool character::can_continue_sliding() const
{
	const rect& r = body_rect();

	// see if there is a solid within reasonable range that we are clinging to.
	const int xpos = face_right() ? r.x2() + 5 : r.x() - 5;
	const int ypos = r.y2();
	return lvl_->solid(xpos, ypos);
}

void character::change_to_stand_frame()
{
	if(type_->stand_up_slope_frame() && type_->stand_down_slope_frame()) {
		const int slope = slope_standing_on();
		if(slope < 0) {
			change_frame(type_->stand_down_slope_frame());
			return;
		}

		if(slope > 0) {
			change_frame(type_->stand_up_slope_frame());
			return;
		}
	}

	change_frame(&type_->get_frame());
}

int character::slope_standing_on(int range) const
{
	if(lvl_ == NULL || !is_standing(*lvl_)) {
		return 0;
	}

	const int forward = face_right() ? 1 : -1;
	const int xpos = feet_x();
	int ypos = feet_y();


	for(int n = 0; !lvl_->solid(xpos, ypos) && n != 10; ++n) {
		++ypos;
	}

	if(range == 1) {
		if(lvl_->solid(xpos + forward, ypos - 1) &&
		   !lvl_->solid(xpos - forward, ypos)) {
			return 45;
		}

		if(!lvl_->solid(xpos + forward, ypos) &&
		   lvl_->solid(xpos - forward, ypos - 1)) {
			return -45;
		}

		return 0;
	} else {
		if(!is_standing(*lvl_)) {
			return 0;
		}

		int y1 = find_ground_level(*lvl_, xpos + forward*range, ypos, range+1);
		int y2 = find_ground_level(*lvl_, xpos - forward*range, ypos, range+1);
		while((y1 == INT_MIN || y2 == INT_MIN) && range > 0) {
			y1 = find_ground_level(*lvl_, xpos + forward*range, ypos, range+1);
			y2 = find_ground_level(*lvl_, xpos - forward*range, ypos, range+1);
			--range;
		}

		if(range == 0) {
			return 0;
		}

		const int dy = y2 - y1;
		const int dx = range*2;
		return (dy*45)/dx;
	}
}

bool character::in_stand_frame() const
{
	return current_frame_ == &type_->get_frame() || current_frame_ == type_->idle_frame() || current_frame_ == type_->stand_up_slope_frame() || current_frame_ == type_->stand_down_slope_frame();
}

void character::change_frame(const frame* new_frame)
{
	if(new_frame == NULL || current_frame_ == type_->die_frame()) {
		return;
	}


	++frame_id_;

	time_in_frame_ = 0;

	const int start_x = feet_x();
	const int start_y = feet_y();

	current_frame_ = new_frame;

	const int diff_x = feet_x() - start_x;
	const int diff_y = feet_y() - start_y;

	set_pos(x() - diff_x, y() - diff_y);

	if(new_frame->velocity_x()) {
		velocity_x_ = new_frame->velocity_x()*(face_right() ? 1 : -1);
	}

	if(new_frame->velocity_y()) {
		velocity_y_ = new_frame->velocity_y();
	}

	new_frame->play_sound();
}

bool character::point_collides(int xpos, int ypos) const
{
	return point_in_rect(point(xpos, ypos), body_rect());
}

void character::move_to_standing(level& lvl)
{
	int start_y = y();
	lvl_ = &lvl;
	for(int n = 0; n != 1000; ++n) {
		if(is_standing(lvl)) {

			if(n == 0) {
				for(int n = 0; n != 1000; ++n) {
					set_pos(x(), y() - 1);
					if(!is_standing(lvl)) {
						set_pos(x(), y() + 1);
						return;
					}
				}
				return;
			}
			return;
		}

		set_pos(x(), y() + 1);
	}

	set_pos(x(), start_y);
}

int character::hitpoints() const
{
	return hitpoints_;
}

int character::max_hitpoints() const
{
	return max_hitpoints_;
}

int character::walk_speed() const
{
	return walk_speed_;
}

int character::jump_power() const
{
	return jump_power_;
}

int character::boost_power() const
{
	return boost_power_;
}

int character::glide_speed() const
{
	return glide_speed_;
}

void character::get_hit()
{
	assert(!invincible_);
	if(is_human()) {
		entity_ptr hitby = lvl_->collide(body_rect(), this);
		if(hitby) {
			hitby->hit_player();
		}
	}

	--hitpoints_;
	invincible_ = invincibility_duration();

	if(hitpoints_ <= 0 && type_->die_frame()) {
		change_frame(type_->die_frame());
	} else if(type_->gethit_frame()) {
		change_frame(type_->gethit_frame());
	}
}

bool character::is_standable(int xpos, int ypos, int* friction, int* adjust_y) const
{
	const frame& f = current_frame();
	if(f.has_platform() == false) {
		return false;
	}

	int y1 = y() + f.platform_y();
	int y2 = previous_y_ + f.platform_y();

	if(y1 > y2) {
		std::swap(y1, y2);
	}

	if(ypos < y1 || ypos > y2) {
		return false;
	}

	if(xpos < x() + f.platform_x() || xpos >= x() + f.platform_x() + f.platform_w()) {
		return false;
	}

	if(friction) {
		*friction = type_->friction();
	}

	if(adjust_y) {
		*adjust_y = y() + f.platform_y() - ypos;
	}

	return true;
}

variant character::get_value(const std::string& key) const
{
	if(key == "x") {
		return variant(body_rect().x());
	} else if(key == "y") {
		return variant(y());
	} else if(key == "x2") {
		return variant(body_rect().x2());
	} else if(key == "y2") {
		return variant(body_rect().y2());
	} else if(key == "facing") {
		return variant(face_right() ? 1 : -1);
	} else if(key == "cycle") {
		return variant(cycle_num_);
	} else if(key == "player") {
		if(lvl_ && lvl_->player()) {
			return variant(lvl_->player().get());
		} else {
			return variant();
		}
	} else if(key == "collided") {
		return variant(collided_since_last_frame_);
	} else if(key == "near_cliff_edge") {
		return variant(is_standing(*lvl_) &&
		               cliff_edge_within(*lvl_, feet_x(), feet_y(), face_dir()*15));
	} else if(key == "last_walk") {
		return variant(walk_result_);
	} else if(key == "last_jump") {
		return variant(jump_result_);
	} else if(key == "hitpoints") {
		return variant(hitpoints());
	} else if(key == "max_hitpoints") {
		return variant(max_hitpoints());
	} else if(key == "walk_speed") {
		return variant(walk_speed());
	} else if(key == "jump_power") {
		return variant(jump_power());
	} else if(key == "glide_speed") {
		return variant(glide_speed());
	} else {
		std::map<std::string, variant>::const_iterator i = vars_.find(key);
		if(i != vars_.end()) {
			return i->second;
		}
		return variant();
	}
}

void character::get_inputs(std::vector<game_logic::formula_input>* inputs) const
{
}

void character::set_value(const std::string& key, const variant& value)
{
	if(key == "hitpoints") {
		hitpoints_ = value.as_int();
	} else if(key == "max_hitpoints") {
		max_hitpoints_ = value.as_int();
	} else if(key == "walk_speed") {
		walk_speed_ = value.as_int();
	} else if(key == "jump_power") {
		jump_power_ = value.as_int();
	} else if(key == "boost_power") {
		boost_power_ = value.as_int();
	} else if(key == "glide_speed") {
		glide_speed_ = value.as_int();
	} else {
		vars_[key] = value;
	}
}

void pc_character::set_value(const std::string& key, const variant& value)
{
	if(key == "save") {
		save_game();
	} else {
		character::set_value(key, value);
	}
}

bool character::body_passthrough() const
{
	return type_->is_vehicle() && driver_.get() == NULL;
}

bool character::body_harmful() const
{
	return !type_->is_vehicle() || driver_.get() != NULL;
}

bool character::boardable_vehicle() const
{
	return type_->is_vehicle() && driver_.get() == NULL;
}

void character::control(const level& lvl)
{
	if(current_frame_ == type_->die_frame()) {
		return;
	}

	if(type_->is_vehicle() && driver_.get() == NULL) {
		return;
	}

	++time_since_last_formula_;
	if(time_since_last_formula_ == formula_test_frequency_) {
		time_since_last_formula_ = 0;
	}

	if(fly_formula_) {
		if(time_since_last_formula_ == 0) {
			fly_result_ = fly_formula_->execute(*this).as_int();
		}
	}

	if(walk_formula_) {
		if(time_since_last_formula_ == 0) {
			walk_result_ = walk_formula_->execute(*this).as_int();
		}

		if(fly_formula_) {
			fly(lvl, walk_result_ < 0, fly_result_);
		} else if(walk_result_ < 0) {
			walk(lvl, false);
		} else if(walk_result_ > 0) {
			walk(lvl, true);
		}
	}

	if(jump_formula_) {
		if(time_since_last_formula_ == 0) {
			jump_result_ = jump_formula_->execute(*this).as_int();
		}
		if(jump_result_) {
			jump(lvl);
		}
	}

	if(xpos_formula_){
		set_pos(xpos_formula_->execute(*this).as_int(), y());
	}

	if(ypos_formula_){
		set_pos(x(), ypos_formula_->execute(*this).as_int());
	}
}

bool pc_character::look_up() const
{
	return key_[SDLK_UP] || joystick::up();
}

bool pc_character::look_down() const
{
	return key_[SDLK_DOWN] || joystick::down();
}

void pc_character::item_destroyed(const std::string& level_id, int item)
{
	items_destroyed_[level_id].push_back(item);
}

const std::vector<int>& pc_character::get_items_destroyed(const std::string& level_id) const
{
	std::vector<int>& v = items_destroyed_[level_id];
	std::sort(v.begin(), v.end());
	return v;
}

void pc_character::object_destroyed(const std::string& level_id, int object)
{
	objects_destroyed_[level_id].push_back(object);
}

const std::vector<int>& pc_character::get_objects_destroyed(const std::string& level_id) const
{
	std::vector<int>& v = objects_destroyed_[level_id];
	std::sort(v.begin(), v.end());
	return v;
}

void pc_character::control(const level& lvl)
{
	if(current_level_ != lvl.id()) {
		//key_.RequireRelease();
		current_level_ = lvl.id();
	}

	if(&current_frame() == type().attack_frame() ||
	   &current_frame() == type().jump_attack_frame() ||
	   &current_frame() == type().up_attack_frame() ||
	   &current_frame() == type().run_attack_frame()) {
		if(&current_frame() == type().run_attack_frame()) {
			set_velocity(velocity_x() + current_frame().accel_x() * (face_right() ? 1 : -1),
			             velocity_y() + current_frame().accel_y());
			running_ = false;
			return;
		}

		running_ = false;
		return;
	}

	if(running_ && !key_[SDLK_LEFT] && !key_[SDLK_RIGHT] && !joystick::left() && !joystick::right()) {
		running_ = false;
	}

	if(key_[SDLK_a] || joystick::button(1) || joystick::button(3)) {
		if(key_[SDLK_DOWN] || joystick::down()) {
			jump_down(lvl);
		} else {
			jump(lvl);
		}
		set_last_jump(true);
	} else {
		set_last_jump(false);
	}

	if(key_[SDLK_DOWN] || joystick::down()) {
		crouch(lvl);
		return;
	} else if(&current_frame() == type().crouch_frame()) {
		uncrouch(lvl);
	}

	if(key_[SDLK_UP] || joystick::up()) {
		lookup(lvl);
	} else if(&current_frame() == type().lookup_frame()) {
		unlookup(lvl);
	}

	if(key_[SDLK_s] || joystick::button(0) || joystick::button(2)) {
		attack(lvl);
		return;
	}

	const int double_tap_cycles = 10;

	if(key_[SDLK_LEFT] || joystick::left()) {
		walk(lvl, false);
		if(!prev_left_ || running_) {
			if(last_left_ > cycle() - double_tap_cycles || running_) {
				run(lvl, false);
				running_ = true;
			}

			last_left_ = cycle();
		}
		prev_left_ = true;
	} else {
		prev_left_ = false;
	}

	if(key_[SDLK_RIGHT] || joystick::right()) {
		walk(lvl, true);
		if(!prev_right_ || running_) {
			if(last_right_ > cycle() - double_tap_cycles || running_) {
				run(lvl, true);
				running_ = true;
			}

			last_right_ = cycle();
		}
		prev_right_ = true;
	} else {
		prev_right_ = false;
	}
}

void pc_character::save_game()
{
		save_condition_ = NULL;
		save_condition_ = new pc_character(*this);
}
