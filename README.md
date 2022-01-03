# Spchcat

Speech recognition tool to convert audio to text transcripts.

## Description

`spchcat` is a command-line tool to read in audio from .WAV files, microphone, or system audio inputs and convert any speech found into text. It runs locally on your machine, with no web API calls or network activity, and is open source. It is built on top of [Coqui's speech to text library](https://github.com/coqui-ai/STT), [TensorFlow](https://www.tensorflow.org/), [KenLM](https://kheafield.com/code/kenlm/), and [data from Mozilla's Common Voice project](https://commonvoice.mozilla.org/).

It supports multiple languages thanks to [Coqui's library of models](https://github.com/coqui-ai/STT-models). The accuracy of the recognized text will vary widely depending on the language, since some have only small amounts of training data. You can help improve future models by [contributing your voice](https://commonvoice.mozilla.org/).

## Installation

On Debian-based x86 Linux systems like Ubuntu you should be able to install [the latest .deb package](https://github.com/petewarden/spchcat/releases/download/v0.0.1-alpha/spchcat_0.0-1_amd64.deb) by downloading and double-clicking it. Other platforms are currently unsupported.

## Usage

After installation, you should be able to run it with no arguments to start capturing audio from the default microphone source, with the results output to the terminal:

```bash
spchcat
```

After you've run the command, start speaking, and you should see the words you're saying appear. The speech recognition is still a work in progress, and the accuracy will depend a lot on the noise levels, your accent, and the complexity of the words, but hopefully you should see something close enough to be useful for simple note taking or other purposes.

### System Audio

If you don't have a microphone attached, or want to transcribe audio coming from another program, you can set the `--source` argument to 'system'. This will attempt to listen to the audio that your machine is playing, including any videos or songs, and transcribe any speech found.

```bash
spchcat --source=system
```

### WAV Files

One of the most common audio file formats is WAV. If you don't have any to test with, you can download [Coqui's test set](https://github.com/coqui-ai/STT/releases/download/v1.1.0/audio-1.1.0.tar.gz) to try this option out. If you need to convert files from another format like '.mp3', I recommend using [FFMPeg](https://www.ffmpeg.org/). As with the other source options, `spchcat` will attempt to find any speech in the files and convert it into a transcript. You don't have to explicitly set the `--source` argument, as long as file names are present on the command line that will be the default.

```bash
spchcat audio/8455-210777-0068.wav 
```

If you're using the audio file from the test set, you should see output like the following:

```bash
TensorFlow: v2.3.0-14-g4bdd3955115
 Coqui STT: v1.1.0-0-gf3605e23
your power is sufficient i said 
```

You can also specify a folder instead of a single filename, and all `.wav` files within that directory will be transcribed.

### Language Support

So far this documentation has assumed you're using American English, but the tool will default to looking for the language your system has been configured to use. It first looks for the one specified in the `LANG` environment variable. If no model for that language is found, it will default back to 'en_US'. You can override this by setting the `--language` argument on the command line, for example:

```bash
spchcat --language=de_DE
```

This works independently of `--source` and other options, so you can transcribe microphone, system audio, or files in any of the supported languages. It should be noted that some languages have very small amounts of data and so their quality may suffer. If you don't care about country-specific variants, you can also just specify the language part of the code, for example `--language=en`. This will pick any model that supports the language, regardless of country. The same thing happens if a particular language and country pair isn't found, it will log a warning and fall back to any country that supports the language. For example, if 'en_GB' is specified but only 'en_US' is present, 'en_US' will be used.

| Language Name | Code    |
| ------------: |:-------------|
|am_ET|Amharic|
|bn_IN|Bengali|
|br_FR|Breton|
|ca_ES|Catalan|
|cnh_MM|Hakha-Chin|
|cs_CZ|Czech|
|cv_RU|Chuvash|
|cy_GB|Welsh|
|de_DE|German|
|dv_MV|Dhivehi|
|el_GR|Greek|
|en_US|English|
|et_EE|Estonian|
|eu_ES|Basque|
|fi_FI|Finnish|
|fr_FR|French|
|fy_NL|Frisian|
|ga_IE|Irish|
|hu_HU|Hungarian|
|id_ID|Indonesian|
|it_IT|Italian|
|ka_GE|Georgian|
|ky_KG|Kyrgyz|
|lg_UG|Luganda|
|lt_LT|Lithuanian|
|lv_LV|Latvian|
|mn_MN|Mongolian|
|mt_MT|Maltese|
|nl_NL|Dutch|
|or_IN|Odia|
|pt_PT|Portuguese|
|rm_CH|Romansh-Sursilvan|
|ro_RO|Romanian|
|ru_RU|Russian|
|rw_RW|Kinyarwanda|
|sah_RU|Sakha|
|sb_DE|Upper-Sorbian|
|sl_SI|Slovenian|
|sw_KE|Swahili-Congo|
|ta_IN|Tamil|
|th_TH|Thai|
|tr_TR|Turkish|
|tt_RU|Tatar|
|uk_UK|Ukrainian|
|wo_SN|Wolof|
|yo_NG|Yoruba|

All of these models have been collected by Coqui, and contributed by organizations like [Inclusive Technology for Marginalized Languages](https://itml.cl.indiana.edu/) or individuals. All are using the conventions for Coqui's STT library, so custom models could potentially be used, but training and deployment of those is outside the scope of this document. The models themselves are provided under a variety of open source licenses, which can be inspected in their source folders (typically inside `/etc/spchcat/models/`).

### Saving Output

By default `spchcat` writes any recognized text to the terminal, but it's designed to behave like a normal Unix command-line tool, so it can also be written to a file using indirection like this:

```bash
spchcat audio/8455-210777-0068.wav > /tmp/transcript.txt
```

If you then run `cat /tmp/transcript.txt` (or open it in an editor) you should see `your power is sufficient i said'. You can also pipe the output to another command. Unfortunately you can't pipe audio into the tool from another executable, since pipes aren't designed for non-text data. 

There is one subtle difference between writing to a file and to the terminal. The transcription itself can take some time to settle into a final form, especially when waiting for long words to finish, so when it's being run live in a terminal you'll often see the last couple of words change. This isn't useful when writing to a file, so instead the output is finalized before it's written. This can introduce a small delay when writing live microphone or system audio input.

## Build from Source

### Tool

It's possible to build all dependencies from source, but I recommending downloading binary versions of Coqui's STT, TensorFlow Lite, and KenLM libraries from [github.com/coqui-ai/STT/releases/download/v1.1.0/native_client.tflite.Linux.tar.xz](https://github.com/coqui-ai/STT/releases/download/v1.1.0/native_client.tflite.Linux.tar.xz). Extract this to a folder, and then from inside a folder containing this repo run to build the `spchcat` tool itself:

```bash
make spchcat LINK_PATH_STT=-L../STT_download
```

You should replace `../STT_download` with the path to the Coqui library folder. After this you should see a `spchcat` executable binary in the repo folder. Because it relies on shared libraries, you'll need to specify a path to these too using `LD_LIBRARY_PATH` unless you have copies in system folders.

```bash
LD_LIBRARY_PATH=../STT_download ./spchcat
```

### Models

The previous step only built the executable binary itself, but for the complete tool you also need data files for each language. If you have the [`gh` GitHub command line tool](https://cli.github.com/) you can run the `download_releases.py` script to fetch [Coqui's releases](https://github.com/coqui-ai/STT-models/releases) into the `build/models` folder in your local repo. You can then run your locally-built tool against these models using the `--languages_dir` option:

```bash
LD_LIBRARY_PATH=../STT_download ./spchcat --languages_dir=build/models/
```

### Installer

After you have the tool built and the model data downloaded, `create_deb_package.sh` will attempt to package them into a Debian installer archive. It will take several minutes to run, and the result ends up in `spchcat_0.0-1_amd64.deb`.

## Contributors

Tool code written by [Pete Warden](https://twitter.com/petewarden), pete@petewarden.com, heavily based on [Coqui's STT example](https://github.com/coqui-ai/STT/blob/main/native_client/stt.cc). It's a pretty thin wrapper on top of [Coqui's speech to text library](https://github.com/coqui-ai/STT), so the Coqui team should get credit for their amazing work. Also relies on [TensorFlow](https://www.tensorflow.org/), [KenLM](https://kheafield.com/code/kenlm/), [data from Mozilla's Common Voice project](https://commonvoice.mozilla.org/), and all the contributors to [Coqui's model zoo](https://coqui.ai/models).

## License

Tool code is licensed under the Mozilla Public License Version 2.0, see LICENSE in this folder.

All other libraries and model data are released under their own licenses, see the relevant folders for more details.
