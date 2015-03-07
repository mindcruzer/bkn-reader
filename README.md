### Batch Kinetics File Reader

This is a small application I made back in the day that pulls data out of .bkn files produced by 
Varian Cary WinUV software, and serializes it to JSON.

I made this because I'm lazy and I wanted to see if I could generate some kinetics graphs automatically.

#### Prereqs

You need to have pcre installed. In Ubuntu or Mint: `sudo apt-get install libpcre3 libpcre3-dev`

#### Use

Clone, cd into the folder, 

    make
    ./read_BKN test_data/test.bkn

Go nuts.
