t=1.2;
w=83;
d=58;
ht=36;
er=2;

$fn=50;

//translate([0,0.4,ht+t-5.6]) cube([6,14.2,5.6]);

difference()
{
    translate([-t,-t,0]) minkowski()
    {
        cube([w+2*t-er,d+2*t-er,ht+t]);
        translate([t,t,0]) cylinder(r=er,h=0.01);
    }
    union() {
        cube([w,d,ht]);
        
        // LCD
        translate([5,20,36]) cube([73,27,10]);
        
        // power socket
        translate([-t-2,2,0]) cube([10,12,16]);

        // USB socket
        translate([-t-2,32,3]) cube([10,12,6]);

        // SD card slot
        translate([-t-2,32,16]) cube([10,16,6]);

        // GPS antenna
        translate([-t-2,12,24]) rotate ([0,90,0]) cylinder(r=3.5,h=10);
        
        // buttons
        translate([0,0,33]) cube([32,15,10]);
        
        // buttons right
        translate([72,0,33]) cube([8,15,10]);
    }
}

translate([-t,-t,-50]) union() {
    minkowski() {
        cube([w+2*t-er,d+2*t-er,t]);
        translate([t,t,0]) cylinder(r=2,h=1);
    }
    translate([t,t,0]) cube([w,0.8,5]);
    translate([t,d,0]) cube([w,0.8,5]);
}