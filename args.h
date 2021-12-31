#ifndef __ARGS_H__
#define __ARGS_H__

#if defined(_MSC_VER)
#include "getopt_win.h"
#else
#include <getopt.h>
#endif
#include <iostream>
#include <sys/stat.h>

#include "coqui-stt.h"

const char* app_name = NULL;

const char* language = NULL;

const std::string models_folder = "/etc/spchcat/models/";

const char* model = NULL;

const char* scorer = NULL;

const char* audio = NULL;

const char* source = "mic";

int source_buffer_size = 160 * 4;

bool set_beamwidth = false;

int beam_width = 0;

bool set_alphabeta = false;

float lm_alpha = 0.f;

float lm_beta = 0.f;

bool show_times = false;

bool has_versions = false;

bool extended_metadata = false;

bool json_output = false;

int json_candidate_transcripts = 3;

int stream_size = 0;

int extended_stream_size = 0;

char* hot_words = NULL;

const std::string ListAvailableLanguages() {
  std::string result;
  DIR* dp = opendir(models_folder.c_str());
  if (dp != NULL)
  {
    struct dirent* ep;
    while (ep = readdir(dp))
    {
      if (result.length() == 0) {
        result += ep->d_name;
      }
      else {
        result += "|";
        result += ep->d_name;
      }
    }
    closedir(dp);
  }
  return result;
}

void PrintHelp(const char* bin)
{
  std::cout <<
    "Usage: " << bin << " [--source mic|system|file] [--language " + ListAvailableLanguages() + "] <WAV files>\n"
    "\n"
    "Convert speech in audio to text transcripts.\n"
    "\n"
    "\t--language\tWhich language to look for (default '" << language << "')\n"
    "\t--source NAME\tName of the audio source device (default 'mic', can also be 'system', 'file')\n"
    "\t--help\t\t\t\tShow help\n"
    "Advanced settings:\n"
    "\t--model MODEL\t\t\tPath to the model (protocol buffer binary file)\n"
    "\t--scorer SCORER\t\t\tPath to the external scorer file\n"
    "\t--source_buffer_size SIZE\tNumber of samples to fetch from source\n"
    "\t--audio AUDIO\t\t\tPath to the audio file to run (WAV format)\n"
    "\t--beam_width BEAM_WIDTH\t\tValue for decoder beam width (int)\n"
    "\t--lm_alpha LM_ALPHA\t\tValue for language model alpha param (float)\n"
    "\t--lm_beta LM_BETA\t\tValue for language model beta param (float)\n"
    "\t-t\t\t\t\tRun in benchmark mode, output mfcc & inference time\n"
    "\t--extended\t\t\tOutput string from extended metadata\n"
    "\t--json\t\t\t\tExtended output, shows word timings as JSON\n"
    "\t--candidate_transcripts NUMBER\tNumber of candidate transcripts to include in JSON output\n"
    "\t--stream size\t\t\tRun in stream mode, output intermediate results\n"
    "\t--extended_stream size\t\t\tRun in stream mode using metadata output, output intermediate results\n"
    "\t--hot_words\t\t\tHot-words and their boosts. Word:Boost pairs are comma-separated\n"
    "\t--version\t\t\tPrint version and exits\n";
  char* version = STT_Version();
  std::cerr << "Coqui STT " << version << "\n";
  STT_FreeString(version);
  exit(1);
}

bool DoesFileExist(const std::string& path) {
  struct stat sb;
  return (stat(path.c_str(), &sb) == 0);
}

void SplitString(std::string const& str, const char delim,
  std::vector<std::string>& out)
{
  size_t start;
  size_t end = 0;

  while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
  {
    end = str.find(delim, start);
    out.push_back(str.substr(start, end - start));
  }
}

bool ProcessArgs(int argc, char** argv)
{
  app_name = argv[0];

  language = getenv("LANG");
  if (language == NULL) {
    language = "en_US";
  }
  else {
    static std::vector<std::string> parts;
    SplitString(language, '.', parts);
    language = parts[0].c_str();
  }

  const char* const short_opts = "s:l:m:o:z:a:b:c:d:tejs:r:R:w:vh";
  const option long_opts[] = {
          {"source", required_argument, nullptr, 's'},
          {"language", required_argument, nullptr, 'l'},
          {"model", required_argument, nullptr, 'm'},
          {"scorer", required_argument, nullptr, 'o'},
          {"source_buffer_size", required_argument, nullptr, 'z'},
          {"audio", required_argument, nullptr, 'a'},
          {"beam_width", required_argument, nullptr, 'b'},
          {"lm_alpha", required_argument, nullptr, 'c'},
          {"lm_beta", required_argument, nullptr, 'd'},
          {"t", no_argument, nullptr, 't'},
          {"extended", no_argument, nullptr, 'e'},
          {"json", no_argument, nullptr, 'j'},
          {"candidate_transcripts", required_argument, nullptr, 150},
          {"stream", required_argument, nullptr, 'r'},
          {"extended_stream", required_argument, nullptr, 'R'},
          {"hot_words", required_argument, nullptr, 'w'},
          {"version", no_argument, nullptr, 'v'},
          {"help", no_argument, nullptr, 'h'},
          {nullptr, no_argument, nullptr, 0}
  };

  while (true)
  {
    const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

    if (-1 == opt)
      break;

    switch (opt)
    {
    case 'l':
      language = optarg;
      break;

    case 's':
      source = optarg;
      break;

    case 'm':
      model = optarg;
      break;

    case 'o':
      scorer = optarg;
      break;

    case 'z':
      source_buffer_size = atoi(optarg);
      break;

    case 'a':
      audio = optarg;
      break;

    case 'b':
      set_beamwidth = true;
      beam_width = atoi(optarg);
      break;

    case 'c':
      set_alphabeta = true;
      lm_alpha = atof(optarg);
      break;

    case 'd':
      set_alphabeta = true;
      lm_beta = atof(optarg);
      break;

    case 't':
      show_times = true;
      break;

    case 'e':
      extended_metadata = true;
      break;

    case 'j':
      json_output = true;
      break;

    case 150:
      json_candidate_transcripts = atoi(optarg);
      break;

    case 'r':
      stream_size = atoi(optarg);
      break;

    case 'R':
      extended_stream_size = atoi(optarg);
      break;

    case 'v':
      has_versions = true;
      break;

    case 'w':
      hot_words = optarg;
      break;

    case 'h': // -h or --help
    case '?': // Unrecognized option
    default:
      PrintHelp(argv[0]);
      break;
    }
  }

  if (has_versions) {
    char* version = STT_Version();
    std::cout << "Coqui " << version << "\n";
    STT_FreeString(version);
    return false;
  }

  if (!model) {
    static std::string model_string = models_folder + language + "/" + "model.tflite";
    if (!DoesFileExist(model_string)) {
      fprintf(stderr, "Warning: Model not found at %s\n", model_string.c_str());
    }
    model = model_string.c_str();
  }

  if (!scorer) {
    static std::string scorer_string = models_folder + language + "/" + "scorer.scorer";
    if (DoesFileExist(scorer_string)) {
      scorer = scorer_string.c_str();
    }
  }

  if ((stream_size < 0 || stream_size % 160 != 0) || (extended_stream_size < 0 || extended_stream_size % 160 != 0)) {
    std::cout <<
      "Stream buffer size must be multiples of 160\n";
    return false;
  }

  return true;
}

#endif // __ARGS_H__
