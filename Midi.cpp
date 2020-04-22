#include "Midi.hpp"


#include <algorithm>
#include <iostream>
#include <map>
#include <thread>
#include <Windows.h>

#include "Util.hpp"

MidiEvent::MidiEvent(const MidiEvent &val)
{
	delta_t = val.delta_t;
	status = val.status;
	channel = val.channel;
	data[0] = val.data[0];
	data[1] = val.data[1];
}

MidiChunk::MidiChunk(std::ifstream & input_file, ChunkType chunk_type) : chunk_type(chunk_type)
{
	char tmp[4];

	input_file.read(tmp, 4);

	//std::cout << "pos: " << std::hex << input_file.tellg() << "\n";
	//std::cout << "temp val: " << tmp[0] << tmp[1] << tmp[2] << tmp[3] << "\n";

	if (chunk_type == ChunkType::Header)
	{
		if (!comp_char_array(tmp, "MThd", 4))
		{
			error_and_exit("Not a midi file.");
		}
	} else if (chunk_type == ChunkType::Track)
	{
		if (!comp_char_array(tmp, "MTrk", 4))
		{
			error_and_exit("Malformed midi file.");
		}
	}

	input_file.read(tmp, 4);
	
	length = char_arr_to_int(tmp, 4);

	//std::cout << "length: " << std::dec << length << "\n";

	if (chunk_type == ChunkType::Header)
	{
		if (length < 6)
		{
			error_and_exit("Malformed midi file.");
		}
		input_file.read(tmp, 2);

		{
			const int val = char_arr_to_int(tmp, 2);

			switch (val)
			{
			case 0:
				format = Format::SingleTrack;
				break;
			case 1:
				format = Format::MultiTrack;
				break;
			case 2:
				format = Format::IndepTracks;
				break;
			default:
				error_and_exit("Malformed midi file.");
			}
		}

		input_file.read(tmp, 2);
		number_tracks = char_arr_to_int(tmp, 2);
		
		input_file.read(tmp, 2);
		ticks_per_quarter_note = char_arr_to_int(tmp, 2);

		if (length > 6)
		{
			input_file.seekg(static_cast<std::ifstream::off_type>(length - 6), std::ios::cur);
		}
	}
	else if (chunk_type == ChunkType::Track)
	{
        int cur_len = 0;
        while (cur_len < length) {
			MidiEvent event;

			event.delta_t = get_var_len_int(input_file, cur_len);

			//std::cout << "spot: " << std::hex << input_file.tellg() << "\n";

        	input_file.read(tmp, 1);
            cur_len++;

			//std::cout << "spot: " << std::hex << input_file.tellg() << "\n";

			int val = ((int)tmp[0] & 0xFF);

			//std::cout << std::hex << "event type: " << val << "\n";

        	if (val == 0xFF) // Meta-Event (Ignore Lmao)
        	{
                input_file.read(tmp, 1);
                cur_len++;

        		int val2 = ((int)tmp[0] & 0xFF);

				//std::cout << std::hex << "message type: " << val2 << "\n";

        		switch(val2)
        		{
				case 0x00:
				case 0x20:
				case 0x2F:
				case 0x51:
				case 0x54:
				case 0x58:
				case 0x59:
				{
					input_file.read(tmp, 1);
					cur_len++;

					int val3 = ((int)tmp[0] & 0xFF);

					input_file.seekg(static_cast<std::ifstream::off_type>(val3), std::ios::cur);
					cur_len += val3;

					break;
				}
				default:
					const int len = get_var_len_int(input_file, cur_len);
					input_file.seekg(static_cast<std::ifstream::off_type>(len), std::ios::cur);
					cur_len += len;
					break;
        		}
        	}
            else if (val == 0xF0 || val == 0xF7) // Sysex-Event (Ignore Lmao)
            {
				int len = get_var_len_int(input_file, cur_len);
				input_file.seekg(static_cast<std::ifstream::off_type>(len), std::ios::cur);
				cur_len += len;
            }
            else if (val >= 0x80 && val < 0xF0) // Midi-Event (The Good Stuff)
            {
				int status = val & 0xF0;
				int channel = val & 0x0F;

				switch(status)
				{
				case 0x80:
					event.status = Status::NoteOff;
					break;
				case 0x90:
					event.status = Status::NoteOn;
					break;
				case 0xA0:
					event.status = Status::KeyPressure;
					break;
				case 0xB0:
					event.status = Status::ControllerChange;
					break;
				case 0xC0:
					event.status = Status::ProgramChange;
					break;
				case 0xD0:
					event.status = Status::ChannelPressure;
					break;
				case 0xE0:
					event.status = Status::PitchBend;
					break;
				default:
					error_and_exit("Malformed midi file.");
					break;
				}

				event.channel = channel;

            	switch(event.status)
            	{
				case Status::NoteOff:
				case Status::NoteOn:
				case Status::KeyPressure:
				case Status::ControllerChange:
				case Status::PitchBend:
					input_file.read(event.data, 2);
					cur_len += 2;
					break;
				case Status::ProgramChange:
				case Status::ChannelPressure:
					input_file.read(event.data, 1);
					cur_len++;
					break;
            	}

				events.push_back(event);
            }
        	else
            {
                error_and_exit("M12alformed midi file.");
            }
        }
	}
}

MidiChunk::MidiChunk(const MidiChunk &val)
{
	chunk_type = val.chunk_type;
	length = val.length;
	events = std::vector<MidiEvent>();
	
	if (chunk_type == ChunkType::Header)
	{
		format = val.format;
		number_tracks = val.number_tracks;
		ticks_per_quarter_note = val.ticks_per_quarter_note;
	}
	else if (chunk_type == ChunkType::Track)
	{
		for (size_t i = 0; i < val.events.size(); ++i)
		{
			events.push_back(val.events[i]);
		}
	}
}

Midi::Midi(std::ifstream & input_file)
{
	chunks.push_back(MidiChunk(input_file, ChunkType::Header));

	while (input_file.good()) {
		if (input_file.peek() == -1)
			break;
		
		chunks.push_back(MidiChunk(input_file, ChunkType::Track));
	}
}

void Midi::play()
{
	std::cout << "Playing...\n";
	
	std::vector<std::tuple<char, int, int, int>> key_sounds;
	std::map<std::tuple<char, int>, int> last_sound;

	uint32_t time = 0;
	
	for (auto& chunk : chunks)
	{
		switch (chunk.chunk_type)
		{
		case ChunkType::Header:
			break;
		case ChunkType::Track:
			for (auto& event : chunk.events)
			{
				time += event.delta_t;
				
				switch (event.status)
				{
				case Status::NoteOff:
					if (last_sound.contains(std::make_tuple(event.data[0], event.channel)))
					{
						const int index = last_sound[std::make_tuple(event.data[0], event.channel)];

						if (index == -1)
						{
							break;
						}

						std::get<2>(key_sounds[index]) = time;

						last_sound[std::make_tuple(event.data[0], event.channel)] = -1;
					}
					break;
				case Status::NoteOn:
					if (last_sound.contains(std::make_tuple(event.data[0], event.channel)))
					{
						const int index = last_sound[std::make_tuple(event.data[0], event.channel)];

						if (index != -1)
						{
							break;
						}
					}

					last_sound[std::make_tuple(event.data[0], event.channel)] = key_sounds.size();

					key_sounds.push_back(std::make_tuple(event.data[0], time, 0, event.channel));

					break;
				case Status::KeyPressure: break;
				case Status::ControllerChange: break;
				case Status::ProgramChange: break;
				case Status::ChannelPressure: break;
				case Status::PitchBend: break;
				default:
					error_and_exit("how?");;
				}
			}
		}
	}

	std::sort(key_sounds.begin(), key_sounds.end(), [](auto a, auto b) {
		return std::get<1>(a) < std::get<1>(b);
	});

	/*for (size_t i = 0; i < key_sounds.size(); ++i)
		std::cout << std::get<0>(key_sounds[i]) << ":" << std::get<1>(key_sounds[i]) << " - " << std::get<2>(key_sounds[i]) << "\n";*/

	for(size_t i = 0; i < key_sounds.size(); ++i)
	{
		float delta_factor = 2.f;
		
		float sleep_length;
		float beep_length = std::get<2>(key_sounds[i]) - std::get<1>(key_sounds[i]);

		if (i == key_sounds.size() - 1)
		{
			sleep_length = beep_length;
		}
		else
		{
			sleep_length = std::get<1>(key_sounds[i + 1]) - std::get<1>(key_sounds[i]);
		}

		sleep_length *= delta_factor;
		beep_length *= delta_factor;

		float hertz = powf(2, (std::get<0>(key_sounds[i]) - 57.f) / 12.f) * 440.f;
		
		std::thread(Beep, static_cast<int>(hertz), beep_length).detach();

		Sleep(sleep_length);
	}

	std::cout << "End of Play\n";

	Sleep(5 * 1000);
}
