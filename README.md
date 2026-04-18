# rm_nuisance

## how to use

rm_nuisance [options] <input> <output>

options:
  -m <model>        path to whisper.cpp ggml model file
                    (default: models/ggml-base.bin)

  -i <input>        input files list
                    (with no flag only one file can be choosen)

  -o <output>       output files list
                    (with no flag output files will be named like this: <input filename>-output.<input file format>)

  --disable-type    [filler|noise|clicks|pauses|breath] disable processing types
                    (default: none are disabled)

  --help or -h      shows this message

examples:
  rm_nuisance interview.mp3 interview_clean.mp3
  rm_nuisance -m ggml-large-v3.bin lecture.wav lecture_clean.wav

## how to build

## how to install required packages

```
$ apt install git make cmake g++ ffmpeg
```

### building and installing whisper
```
git clone https://github.com/ggerganov/whisper.cpp
cd whisper.cpp
make
sudo cp build/src/libwhisper.so /usr/local/lib/
sudo ldconfig
```