#include <cmath>

#include "../include/motion_profile_generator.h"

static MotionProfile* generate_inverted_profile(MotionState* initial_state, ProfileTarget target, MotionConstraints constraints){
    initial_state->invert();
    MotionProfile* profile = generate_profile(initial_state, get_inverted_target(target), constraints);
    vector<MotionSegment*> copy;
    for(MotionSegment* s : profile->get_segments()){
        copy.push_back(new MotionSegment(s));
    }
    delete profile;
    for(MotionSegment* s : copy){
        s->get_initial()->invert();
        s->get_final()->invert();
    }
    MotionProfile* rv = new MotionProfile(copy);
    return rv;
}

MotionProfile* generate_profile(MotionState* initial_state, ProfileTarget target, MotionConstraints constraints){
    const double dist = target.pos-initial_state->get_pos();
    if (dist < 0 || (dist == 0 && initial_state->get_vel() < 0)){
        return generate_inverted_profile(initial_state, target, constraints);
    }
    MotionProfile* profile = new MotionProfile();
    //clamp initial state values
    MotionState* clamped_initial_state = new MotionState(initial_state->get_pos(), min(initial_state->get_vel(), constraints.max_vel), min(initial_state->get_accel(), constraints.accel), initial_state->get_time());	
    delete initial_state;
    initial_state = clamped_initial_state;
    const double total_dist = target.pos - initial_state->get_pos();
    const double calculated_max_vel = sqrt(initial_state->get_vel2() + target.vel*target.vel)/2.0 + total_dist*constraints.accel;
    //calculate max velocity
    const double max_vel = min(constraints.max_vel, calculated_max_vel);
    bool flag = false;
    //we can accelerate
    if (initial_state->get_vel() < max_vel){
            const double accel_time = (max_vel - initial_state->get_vel())/constraints.accel;
            MotionState* segment_start = new MotionState(initial_state->get_pos(), initial_state->get_vel(), constraints.accel, initial_state->get_time());
            MotionState* segment_end = segment_start->extrapolate(segment_start->get_time() + accel_time);
            profile->add_segment(new MotionSegment(segment_start, segment_end));
            flag = true;
            delete initial_state;
            initial_state = profile->final_state();
    }
    //if we accelerated, initial state is now the state we are at when we stopped accelerating
    double decel_dist = (initial_state->get_vel2() - target.vel*target.vel)/(2.0*constraints.accel);
    decel_dist = max(decel_dist, 0.0);
    const double cruise_dist = max(0.0, target.pos - initial_state->get_pos() - decel_dist);
    if (cruise_dist > 0){
            const double cruise_time = cruise_dist/initial_state->get_vel();
            MotionState* segment_start = new MotionState(initial_state->get_pos(), initial_state->get_vel(), 0, initial_state->get_time());
            MotionState* segment_end = segment_start->extrapolate(segment_start->get_time() + cruise_time);
            profile->add_segment(new MotionSegment(segment_start, segment_end));
            if (!flag) delete initial_state;
            flag = true;
            initial_state = profile->final_state();
    }

    if (decel_dist > 0){
            const double decel_time = (initial_state->get_vel() - target.vel)/constraints.accel;	
            MotionState* segment_start = new MotionState(initial_state->get_pos(), initial_state->get_vel(), -constraints.accel, initial_state->get_time());
            MotionState* segment_end = segment_start->extrapolate(segment_start->get_time() + decel_time);
            profile->add_segment(new MotionSegment(segment_start, segment_end));
            if (!flag) delete initial_state;
    }
    return profile;
}

ProfileTarget get_inverted_target(ProfileTarget target){
    ProfileTarget rv;
    rv.pos = -target.pos;
    rv.vel = -target.vel;
    return rv;
}
