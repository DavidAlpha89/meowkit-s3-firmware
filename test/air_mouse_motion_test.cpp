#include "../src/app/app_06/air_mouse_motion.h"
#include <cassert>
#include <cstdio>
#include <limits>

int slow(float dt) {
    air_mouse::Motion m;
    int total=0;
    for(int i=0;i<static_cast<int>(2.f/dt+.5f);++i) {
        m.add(.5f,0,dt,50);
        auto r=m.pending(); total+=r.x; m.sent(r);
    }
    return total;
}
int main() {
    // A slow rotation must survive the noise gate and retain fractions at
    // different sample rates, rather than changing its effective dead zone.
    int a=slow(.005f), b=slow(.01f), c=slow(.02f);
    assert(a>=34 && a<=35 && std::abs(a-b)<=1 && std::abs(b-c)<=1);
    air_mouse::Motion m;
    for(int i=0;i<1000;++i) m.add(.05f,-.05f,.01f,50);
    assert(m.pending().x==0 && m.pending().y==0);
    m.add(400,200,.01f,50);
    auto r=m.pending(); assert(r.x==127 && r.y>=62 && r.y<=64);
    const float before=m.x; m.sent(r);
    assert(std::fabs(m.x-(before-r.x))<.001f && m.pending().x>0);
    m.clear(); m.add(-400,-200,.01f,50);
    r=m.pending(); assert(r.x==-127 && r.y<=-62 && r.y>=-64);
    m.add(10000,5000,.01f,50); assert(std::fabs(m.x)<=254.01f);
    m.add(1,1,.2f,50); assert(m.x==0 && m.y==0);
    m.add(std::numeric_limits<float>::quiet_NaN(),1,.01f,50);
    assert(m.x==0 && m.y==0);
    air_mouse::Calibration cal;
    for(int i=0;i<149;++i) assert(!cal.add(.2f,-.1f,.05f,1));
    assert(cal.add(.2f,-.1f,.05f,1));
    assert(std::fabs(cal.mean[0]-.2f)<.0001f);
    cal.reset();
    for(int i=0;i<100;++i) cal.add(.2f,0,0,1);
    assert(!cal.add(20,0,0,1) && cal.count==0);
    for(int i=0;i<150;++i) assert(!cal.add(i%2?.4f:-.4f,0,0,1));
    assert(cal.count==0);
    assert(!cal.add(0,0,0,1.3f));
    assert(!cal.add(0,0,std::numeric_limits<float>::infinity(),1));
    // Regression: stationary sensors need not start within 0.8 dps of zero.
    cal.reset();
    for(int i=0;i<149;++i) assert(!cal.add(2.f+(i%2?.03f:-.03f),-1.2f,.9f,1));
    assert(cal.add(2.03f,-1.2f,.9f,1));
    assert(std::fabs(cal.mean[0]-2.f)<.001f);
    std::puts("Air Mouse: slow motion, timing, drift gate, vector limits, backlog and calibration passed");
}
