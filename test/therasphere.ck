
Blit s => LPF f => dac;

10 => s.freq;

OscRecv recv;
7775 => recv.port;
recv.listen();

OscSend xmit;
xmit.setHost( "localhost", 7774 );

fun void listen_pos()
{
    recv.event( "/world/s/position","fff" ) @=> OscEvent oe;

    float x, y, z, mag, lx, ly, lz;

    while (1) {
        oe => now;

        while ( oe.nextMsg() != 0 )
        {
            oe.getFloat() => x;
            oe.getFloat() => y;
            oe.getFloat() => z;
        }

        Math.sqrt(x*x + y*y + z*z) => mag;

        // Set the oscillator to a related frequency
        Math.min(mag,1) => s.gain;
        y * 2000 + 440 => s.freq;
        (1-x) * 2000 + 440 => f.freq;

        (x - lx)/0.01 => float vx;
        (y - ly)/0.01 => float vy;
        (z - lz)/0.01 => float vz;

        // Push it back to the center
        xmit.startMsg("/world/s/force","fff");
        -10*x-0.2*vx => xmit.addFloat;
        -10*y-0.2*vy => xmit.addFloat;
        -10*z-0.2*vz => xmit.addFloat;

        x => lx;
        y => ly;
        z => lz;
    }
}

spork ~ listen_pos();

// Create a sphere
xmit.startMsg("/world/sphere/create","sfff");
"s" => xmit.addString;
0 => xmit.addFloat;
0 => xmit.addFloat;
0 => xmit.addFloat;

// Set its size
xmit.startMsg("/world/s/radius","f");
0.2 => xmit.addFloat;

// Set its density
xmit.startMsg("/world/s/density","f");
10 => xmit.addFloat;

// Request its position
xmit.startMsg("/world/s/position/get","i");
10 => xmit.addInt;

// Enable commented lines below if you don't have a haptic controller
// to interact with the object.

while (1) {
/*
    // Push it
    xmit.startMsg("/world/s/force","fff");
    15*Std.rand2f(-1,1) => xmit.addFloat;
    15*Std.rand2f(-1,1) => xmit.addFloat;
    15*Std.rand2f(-1,1) => xmit.addFloat;
*/

    3::second => now;
}
