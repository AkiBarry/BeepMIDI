#pragma once

#include <fstream>
#include <vector>

enum class ChunkType
{
	Header, Track
};

enum class Format
{
	SingleTrack, MultiTrack, IndepTracks
};

enum class Status
{
	NoteOff, NoteOn, KeyPressure, ControllerChange, ProgramChange, ChannelPressure, PitchBend
};

class MidiEvent
{
public:
	MidiEvent() = default;
	MidiEvent(const MidiEvent&);
	
	
	int delta_t;
	Status status;
	uint8_t channel;
	char data[2] = {0, 0};
};

class MidiChunk
{
public:
	MidiChunk(std::ifstream& input_file, ChunkType chunk_type);

	MidiChunk(const MidiChunk&);
	
	~MidiChunk() {}

	ChunkType chunk_type;
	uint32_t length;

	//union {
		struct
		{
			Format format;
			int number_tracks;
			int ticks_per_quarter_note;
		};

		std::vector<MidiEvent> events;
	//};
};

class Midi
{
private:
	std::vector<MidiChunk> chunks;
	
public:
	Midi(std::ifstream& input_file);
	
	void play();
};