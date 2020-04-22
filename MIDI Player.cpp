// MIDI Player.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <fstream>
#include <iostream>


#include "Midi.hpp"
#include "Util.hpp"

int main(int argc, char* argv[])
{	
	if (argc != 2)
	{
		error_and_exit("Drag a midi file onto this executable to play it.");
	}

	std::ifstream midi_file;
	midi_file.open(argv[1]);
	{
		Midi song(midi_file);
		song.play();
	}
	midi_file.close();
}