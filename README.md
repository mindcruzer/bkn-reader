### Cary WinUV File Reader (.BKN)

This application will read the data out of .BKN files produced by Varian Cary WinUV software. The output 
is formatted as JSON. There are no dependencies, so this should build with any standard C compiler.

### Use
To build, `cd` to the root folder and run `make`.

You can verify it's working by reading the sample data file:
```
./bkn_reader sample_data/test.bkn > test.json
```
If you open `test.json` with a text editor you should see the absorbance data in JSON format.