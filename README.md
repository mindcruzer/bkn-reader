### Cary WinUV File Reader (.BKN)

This application will read the data out of .BKN files produced by Varian Cary WinUV software. 
There are no dependencies, so this should build with any standard C compiler.

### Use
To build, `cd` to the root folder and run `make`.

You can verify it's working by reading the sample data file:
```
./bkn_reader sample_data/test.bkn > test.bkn.txt
```
If you open `test.bkn.txt` with a text editor you should see the absorbance points and metadata for each method.