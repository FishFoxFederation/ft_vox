#pragma once

#include <vector>
#include <string>

const std::string sound_folder = "assets/sounds/";
const std::vector<std::string> sound_files = {
	"ping.wav",
	"calm1.wav",
	"grass1.wav"
};

enum class SoundName
{
	PING,
	CALM1,
	GRASS1
};
