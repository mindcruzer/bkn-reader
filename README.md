### Batch Kinetics File Reader

This is a small application I made back in the day that pulls data out of .bkn files produced by 
Varian Cary WinUV software, and serializes it to JSON.

Why? I'm not really sure. I was bored I guess.  

It's kind of... ahem... messy, but, it works, so that's something.

#### Prereqs

You need to have pcre installed. In Ubuntu or Mint: `sudo apt-get install libpcre3 libpcre3-dev`

#### Use

Clone, cd into the folder, 

    make
    ./read_BKN test_data/test.bkn

Go nuts.
