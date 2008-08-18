#ifndef CHARACTER_TYPE_HPP_INCLUDED
#define CHARACTER_TYPE_HPP_INCLUDED

#include <string>

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"

#include "frame.hpp"
#include "wml_node.hpp"

class character_type;

typedef boost::shared_ptr<character_type> character_type_ptr;
typedef boost::shared_ptr<const character_type> const_character_type_ptr;

class character_type
{
public:
	static void init(wml::const_node_ptr node);
	static const_character_type_ptr get(const std::string& id);
	explicit character_type(wml::const_node_ptr node);

	const std::string& id() const { return id_; }
	const frame& get_frame() const { return stand_; }
	const frame* stand_up_slope_frame() const { return stand_up_slope_frame_.get(); }
	const frame* stand_down_slope_frame() const { return stand_down_slope_frame_.get(); }
	const frame* idle_frame() const { return idle_frame_.get(); }
	const frame* turn_frame() const { return turn_frame_.get(); }
	const frame* walk_frame() const { return walk_frame_.get(); }
	const frame* run_frame() const { return run_frame_.get(); }
	const frame* jump_frame() const { return jump_frame_.get(); }
	const frame* fall_frame() const { return fall_frame_.get(); }
	const frame* crouch_frame() const { return crouch_frame_.get(); }
	const frame* lookup_frame() const { return lookup_frame_.get(); }
	const frame* gethit_frame() const { return gethit_frame_.get(); }
	const frame* attack_frame() const { return attack_frame_.get(); }
	const frame* jump_attack_frame() const { return jump_attack_frame_.get(); }
	const frame* up_attack_frame() const { return up_attack_frame_.get(); }
	const frame* run_attack_frame() const { return run_attack_frame_.get(); }
	const frame* die_frame() const { return die_frame_.get(); }
	const frame* fly_frame() const { return fly_frame_.get(); }
	const frame* slide_frame() const { return slide_frame_.get(); }
	const frame* spring_frame() const { return spring_frame_.get(); }
	int walk() const { return walk_; }
	int jump() const { return jump_; }
	int boost() const { return boost_; }
	int glide() const { return glide_; }
	int climb() const { return climb_; }
	int hitpoints() const { return hitpoints_; }
	int springiness() const { return springiness_; }
	int friction() const { return friction_; }
	int traction() const { return traction_; }
	bool is_vehicle() const { return is_vehicle_; }
	int passenger_x() const { return passenger_x_; }
	int passenger_y() const { return passenger_y_; }
private:
	std::string id_;
	frame stand_;
	typedef boost::scoped_ptr<frame> frame_ptr;
	frame_ptr stand_up_slope_frame_, stand_down_slope_frame_, idle_frame_,
	          turn_frame_, walk_frame_, run_frame_, jump_frame_, fall_frame_,
	          crouch_frame_, lookup_frame_, gethit_frame_,
			  attack_frame_, jump_attack_frame_,
			  up_attack_frame_, run_attack_frame_,
			  die_frame_, fly_frame_, slide_frame_, spring_frame_;
	int walk_;
	int jump_;
	int boost_;
	int glide_;
	int climb_;
	int hitpoints_;
	int springiness_;
	int friction_;
	int traction_;
	bool is_vehicle_;
	int passenger_x_, passenger_y_;
};

#endif
