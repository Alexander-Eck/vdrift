#include "sound.h"
#include "unittest.h"
#include "endian_utility.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>

#include <sstream>
using std::stringstream;

#include <list>
using std::list;

#include <iostream>
using std::endl;

#include <string>
using std::string;

#ifdef __APPLE__
#include <Vorbis/vorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif

bool SOUNDBUFFER::LoadWAV(const string & filename, const SOUNDINFO & sound_device_info, std::ostream & error_output)
{
	if (loaded)
		Unload();

	name = filename;

	FILE *fp;

	unsigned int size;

	fp = fopen(filename.c_str(), "rb");
	if (fp)
	{
		char id[5]; //four bytes to hold 'RIFF'

		if (fread(id,sizeof(char),4,fp) != 4) return false; //read in first four bytes
		id[4] = '\0';
		if (!strcmp(id,"RIFF"))
		{ //we had 'RIFF' let's continue
			if (fread(&size,sizeof(unsigned int),1,fp) != 1) return false; //read in 32bit size value
			size = ENDIAN_SWAP_32(size);
			if (fread(id,sizeof(char),4,fp)!= 4) return false; //read in 4 byte string now
			if (!strcmp(id,"WAVE"))
			{ //this is probably a wave file since it contained "WAVE"
				if (fread(id,sizeof(char),4,fp)!= 4) return false; //read in 4 bytes "fmt ";
				if (!strcmp(id,"fmt "))
				{
					unsigned int format_length, sample_rate, avg_bytes_sec;
					short format_tag, channels, block_align, bits_per_sample;

					if (fread(&format_length, sizeof(unsigned int),1,fp) != 1) return false;
					format_length = ENDIAN_SWAP_32(format_length);
					if (fread(&format_tag, sizeof(short), 1, fp) != 1) return false;
					format_tag = ENDIAN_SWAP_16(format_tag);
					if (fread(&channels, sizeof(short),1,fp) != 1) return false;
					channels = ENDIAN_SWAP_16(channels);
					if (fread(&sample_rate, sizeof(unsigned int), 1, fp) != 1) return false;
					sample_rate = ENDIAN_SWAP_32(sample_rate);
					if (fread(&avg_bytes_sec, sizeof(unsigned int), 1, fp) != 1) return false;
					avg_bytes_sec = ENDIAN_SWAP_32(avg_bytes_sec);
					if (fread(&block_align, sizeof(short), 1, fp) != 1) return false;
					block_align = ENDIAN_SWAP_16(block_align);
					if (fread(&bits_per_sample, sizeof(short), 1, fp) != 1) return false;
					bits_per_sample = ENDIAN_SWAP_16(bits_per_sample);


					//new wave seeking code
					//find data chunk
					bool found_data_chunk = false;
					long filepos = format_length + 4 + 4 + 4 + 4 + 4;
					int chunknum = 0;
					while (!found_data_chunk && chunknum < 10)
					{
						fseek(fp, filepos, SEEK_SET); //seek to the next chunk
						if (fread(id, sizeof(char), 4, fp) != 4) return false; //read in 'data'
						if (fread(&size, sizeof(unsigned int), 1, fp) != 1) return false; //how many bytes of sound data we have
						size = ENDIAN_SWAP_32(size);
						if (!strcmp(id,"data"))
						{
							found_data_chunk = true;
						}
						else
						{
							filepos += size + 4 + 4;
						}

						chunknum++;
					}

					if (chunknum >= 10)
					{
						//cerr << __FILE__ << "," << __LINE__ << ": Sound file contains more than 10 chunks before the data chunk: " + filename << endl;
						error_output << "Couldn't find wave data in first 10 chunks of " << filename << endl;
						return false;
					}

					sound_buffer = new char[size];

					if (fread(sound_buffer, sizeof(char), size, fp) != size) return false; //read in our whole sound data chunk

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
					if (bits_per_sample == 16)
					{
						for (unsigned int i = 0; i < size/2; i++)
						{
							//cout << "preswap i: " << sound_buffer[i] << "preswap i+1: " << sound_buffer[i+1] << endl;
							//short preswap = ((short *)sound_buffer)[i];
							((short *)sound_buffer)[i] = ENDIAN_SWAP_16(((short *)sound_buffer)[i]);
							//cout << "postswap i: " << sound_buffer[i] << "postswap i+1: " << sound_buffer[i+1] << endl;
							//cout << (int) i << "/" << (int) size << endl;
							//short postswap = ((short *)sound_buffer)[i];
							//cout << preswap << "/" << postswap << endl;

						}
					}
					//else if (bits_per_sample != 8)
					else
					{
						error_output << "Sound file with " << bits_per_sample << " bits per sample not supported" << endl;
						return false;
					}
#endif

					info = SOUNDINFO(size/(bits_per_sample/8), sample_rate, channels, bits_per_sample/8);
					SOUNDINFO original_info(size/(bits_per_sample/8), sample_rate, channels, bits_per_sample/8);

					loaded = true;
					SOUNDINFO desired_info(original_info.GetSamples(), sound_device_info.GetFrequency(), original_info.GetChannels(), sound_device_info.GetBytesPerSample());
					//ConvertTo(desired_info);
					if (desired_info == original_info)
					{

					}
					else
					{
						error_output << "SOUND FORMAT:" << endl;
						original_info.DebugPrint(error_output);
						error_output << "DESIRED FORMAT:" << endl;
						desired_info.DebugPrint(error_output);
						//throw EXCEPTION(__FILE__, __LINE__, "Sound file isn't in desired format: " + filename);
						//cerr << __FILE__ << "," << __LINE__ << ": Sound file isn't in desired format: " + filename << endl;
						error_output << "Sound file isn't in desired format: "+filename << endl;
						return false;
					}
				}
				else
				{
					//throw EXCEPTION(__FILE__, __LINE__, "Sound file doesn't have \"fmt \" header: " + filename);
					//cerr << __FILE__ << "," << __LINE__ << ": Sound file doesn't have \"fmt \" header: " + filename << endl;
					error_output << "Sound file doesn't have \"fmt\" header: "+filename << endl;
					return false;
				}
			}
			else
			{
				//throw EXCEPTION(__FILE__, __LINE__, "Sound file doesn't have WAVE header: " + filename);
				//cerr << __FILE__ << "," << __LINE__ << ": Sound file doesn't have WAVE header: " + filename << endl;
				error_output << "Sound file doesn't have WAVE header: "+filename << endl;
				return false;
			}
		}
		else
		{
			//throw EXCEPTION(__FILE__, __LINE__, "Sound file doesn't have RIFF header: " + filename);
			//cerr << __FILE__ << "," << __LINE__ << ": Sound file doesn't have WAVE header: " + filename << endl;
			error_output << "Sound file doesn't have RIFF header: "+filename << endl;
			return false;
		}
	}
	else
	{
		//throw EXCEPTION(__FILE__, __LINE__, "Can't open sound file: " + filename);
		//cerr << __FILE__ << "," << __LINE__ << ": Can't open sound file: " + filename << endl;
		error_output << "Can't open sound file: "+filename << endl;
		return false;
	}

	//cout << size << endl;
	return true;
}

bool SOUNDBUFFER::LoadOGG(const string & filename, const SOUNDINFO & sound_device_info, std::ostream & error_output)
{
	if (loaded)
		Unload();

	name = filename;

	FILE *fp;

	unsigned int samples;

	fp = fopen(filename.c_str(), "rb");
	if (fp)
	{
		vorbis_info *pInfo;
		OggVorbis_File oggFile;

		ov_open_callbacks(fp, &oggFile, NULL, 0, OV_CALLBACKS_DEFAULT);

		pInfo = ov_info(&oggFile, -1);

		//I assume ogg is always 16-bit (2 bytes per sample) -Venzon
		samples = ov_pcm_total(&oggFile,-1);
		info = SOUNDINFO(samples*pInfo->channels, pInfo->rate, pInfo->channels, 2);
		
		SOUNDINFO desired_info(info.GetSamples(), sound_device_info.GetFrequency(), info.GetChannels(), sound_device_info.GetBytesPerSample());

		if (!(desired_info == info))
		{
			error_output << "SOUND FORMAT:" << endl;
			info.DebugPrint(error_output);
			error_output << "DESIRED FORMAT:" << endl;
			desired_info.DebugPrint(error_output);

			error_output << "Sound file isn't in desired format: "+filename << endl;
			ov_clear(&oggFile);
			return false;
		}

		//allocate space
		unsigned int size = info.GetSamples()*info.GetChannels()*info.GetBytesPerSample();
		sound_buffer = new char[size];
		int bitstream;
		int endian = 0; //0 for Little-Endian, 1 for Big-Endian
		int wordsize = 2; //again, assuming ogg is always 16-bits
		int issigned = 1; //use signed data

		int bytes = 1;
		unsigned int bufpos = 0;
		while (bytes > 0)
		{
			bytes = ov_read(&oggFile, sound_buffer+bufpos, size-bufpos, endian, wordsize, issigned, &bitstream);
			bufpos += bytes;
			//cout << bytes << "...";
		}

		loaded = true;

		//note: no need to call fclose(); ov_clear does it for us
		ov_clear(&oggFile);

		return true;
	}
	else
	{
		error_output << "Can't open sound file: "+filename << endl;
		return false;
	}
}
/*
const TESTER & SOUND::Test()
{
	int points = 0;

	//test loading a wave file
	SOUNDBUFFER testbuffer;
	SOUNDINFO sinfo;
	sinfo.Set(4096, 44100, 2, 2);

	string testogg = "test/oggtest";
	testbuffer.Load(PATHMANAGER_FUNCTION_CALL(testogg+".ogg"), sinfo);
	points = 0;
	if (testbuffer.GetSoundInfo().GetFrequency() == 44100) points++;
	if (testbuffer.GetSoundInfo().GetBytesPerSample() == 2) points++;
	if (testbuffer.GetSoundInfo().GetChannels() == 2) points++;
	if (testbuffer.GetSoundInfo().GetSamples() == 1412399) points++;
	if (testbuffer.GetSample16bit(1,500000) == -4490) points++;
	//cout << testbuffer.GetSample16bit(1,500000) << endl;
	mytest.SubTestComplete("OGG Vorbis loading", points, 5);

	string testwav = "test/44k_s16_c2";
	testbuffer.Load(PATHMANAGER_FUNCTION_CALL(testwav+".wav"), sinfo);
	points = 0;
	if (testbuffer.GetSoundInfo().GetFrequency() == 44100) points++;
	if (testbuffer.GetSoundInfo().GetBytesPerSample() == 2) points++;
	if (testbuffer.GetSoundInfo().GetChannels() == 2) points++;
	if (testbuffer.GetSoundInfo().GetSamples() == 35952) points++;
	if (testbuffer.GetSample16bit(1,1024) == 722) points++;
	mytest.SubTestComplete("Wave loading 44k s16 c2", points, 5);

	SOUNDSOURCE testsrc;
	testsrc.SetBuffer(testbuffer);
	points = 0;
	testsrc.SeekToSample(1024);
	int out1, out2;
	testsrc.SampleAndAdvance16bit(&out1, &out2, 1);
	//cout << out1 << "," << out2 << endl;
	if (out1 == 0 && out2 == 0) points++;// else cout << "fail1" << endl;
	testsrc.Play();
	testsrc.SeekToSample(1014);
	testsrc.Sample16bit(10, out1, out2);
	if (out1 == 722 && out2 == -224) points++;// else cout << "fail2" << endl;
	testsrc.Advance(10);
	testsrc.SampleAndAdvance16bit(&out1, &out2, 1);
	if (out1 == 722 && out2 == -224) points++;// else cout << "fail3" << endl;
	//cout << out1 << "," << out2 << endl;
	mytest.SubTestComplete("Sound source testing", points, 3);

	SDL_Init(0);
	{
		points = 0;
		testsrc.SeekToSample(0);
		testsrc.SetLoop(true);
		unsigned int numsources = 32;
		unsigned int startticks = SDL_GetTicks();
		for (unsigned int i = 0; i < 44100; i++)
		{
			int accum1 = 0;
			int accum2 = 0;
			for (unsigned int s = 0; s < numsources; s++)
			{
				//testsrc.SampleAndAdvance16bit(out1, out2);
				testsrc.Sample16bit(i, out1, out2);
				accum1 += out1;
				accum2 += out2;
			}
		}
		unsigned int endticks = SDL_GetTicks();
		stringstream msg;
		msg << "Performance testing Sample16bit (" << endticks - startticks << "ms for 1 second of 44.1khz stereo audio with 32 sources)";
		if (endticks - startticks < 500)
			points++;
		mytest.SubTestComplete(msg.str(), points, 1);
	}
	{
		points = 0;
		testsrc.SeekToSample(0);
		//testsrc.SetLoop(false);
		//testsrc.SetLoop(true);
		unsigned int numsources = 32;
		unsigned int startticks = SDL_GetTicks();
		short stream[44100*2];
		int buffer1[44100];
		int buffer2[44100];
		for (unsigned int s = 0; s < numsources; s++)
		{
			testsrc.SampleAndAdvance16bit(buffer1, buffer2, 44100);
			if (s == 0)
			{
				for (int i = 0; i < 44100; i++)
				{
					int pos = i*2;
					((short *) stream)[pos] = buffer1[i];
					((short *) stream)[pos+1] = buffer2[i];
				}
			}
		}
		unsigned int endticks = SDL_GetTicks();
		stringstream msg;
		msg << "Performance testing SampleAndAdvance16bit (" << endticks - startticks << "ms for 1 second of 44.1khz stereo audio with 32 sources)";
		if (endticks - startticks < 500)
			points++;
		mytest.SubTestComplete(msg.str(), points, 1);
		testsrc.SetLoop(true);
		//cout << "Done" << endl;
	}
	{
		points = 0;
		testsrc.SeekToSample(0);
		testsrc.SetLoop(true);
		testsrc.SetPitch(0.7897878);
		//testsrc.SetPitch(0.1);
		unsigned int numsources = 32;
		unsigned int startticks = SDL_GetTicks();
		short stream[44100*2];
		int buffer1[44100];
		int buffer2[44100];
		for (unsigned int s = 0; s < numsources; s++)
		{
			testsrc.SampleAndAdvanceWithPitch16bit(buffer1, buffer2, 44100);
			if (s == 0)
			{
				for (int i = 0; i < 44100; i++)
				{
					int pos = i*2;
					((short *) stream)[pos] = buffer1[i];
					((short *) stream)[pos+1] = buffer2[i];
				}
			}
		}

		unsigned int endticks = SDL_GetTicks();
		stringstream msg;
		msg << "Performance testing SampleAndAdvanceWithPitch16bit (" << endticks - startticks << "ms for 1 second of 44.1khz stereo audio with 32 sources)";
		if (endticks - startticks < 500)
			points++;
		mytest.SubTestComplete(msg.str(), points, 1);
	}
	{
		points = 0;
		testsrc.SeekToSample(0);
		testsrc.SetLoop(true);
		testsrc.SetPitch(0.7897878);
		SOUNDFILTER filter;
		filter.SetFilterOrder1(0.1,0,0.9);
		unsigned int numsources = 32;
		unsigned int startticks = SDL_GetTicks();
		short stream[44100*2];
		int buffer1[44100];
		int buffer2[44100];
		for (unsigned int s = 0; s < numsources; s++)
		{
			testsrc.SampleAndAdvanceWithPitch16bit(buffer1, buffer2, 44100);
			filter.Filter(buffer1, buffer2, 44100);
			if (s == 0)
			{
				for (int i = 0; i < 44100; i++)
				{
					int pos = i*2;
					((short *) stream)[pos] = buffer1[i];
					((short *) stream)[pos+1] = buffer2[i];
				}
			}
		}

		unsigned int endticks = SDL_GetTicks();
		stringstream msg;
		msg << "Performance testing SampleAndAdvanceWithPitch16bit with 1st order filter (" << endticks - startticks << "ms for 1 second of 44.1khz stereo audio with 32 sources)";
		if (endticks - startticks < 1000)
			points++;
		mytest.SubTestComplete(msg.str(), points, 1);
	}
	SDL_Quit();

	testsrc.SeekToSample(1014);
	testsrc.Sample16bit(10, out1, out2);
	deviceinfo = sinfo;
	LoadBuffer(testwav);
	SOUNDSOURCE & testsrc2 = NewSource(testwav);
	testsrc2.SetPitch(0.7);
	testsrc2.SetLoop(true);
	testsrc2.Play();
	{
		int numsamp = 512;
		Uint8 stream[numsamp];
		loaded = true;
		initdone = true;
		testsrc.Play();
		Callback16bitstereo(NULL, stream, numsamp);
		testsrc.Stop();
		initdone = false;
		loaded = false;
		ofstream testdump;
		testdump.open(PATHMANAGER_FUNCTION_CALL("test/testdump.txt").c_str());
		ifstream testgoal;
		testgoal.open(PATHMANAGER_FUNCTION_CALL("test/testgoal.txt").c_str());
		//points = 256*2;
		points = 0;
		for (int i = 0; i < numsamp; i++)
		{
			short goal;
			testgoal >> goal;
			testdump << dec << (short)stream[i] << endl;
			if (stream[i] - goal < 2 && goal - stream[i] < 2)
				points++;
		//	else
		//		cout << (short)goal << " != " << (short)stream[i] << endl;
		}
		testdump.close();
		mytest.SubTestComplete("Sound callback testing", points, numsamp);
	}
	points = 0;
	testsrc2.SeekToSample(1024);
	int out3, out4;
	testsrc2.SampleAndAdvance16bit(&out3, &out4, 1);
	//cout << out1 << "," << out2 << endl;
	if (out1 == out3 && out2 == out4) points++;
	SOUNDSOURCE & testsrc3 = NewSource(testwav);
	testsrc3.Play();
	list <SOUNDSOURCE *> active_sourcelist;
	list <SOUNDSOURCE *> inactive_sourcelist;
	DetermineActiveSources(active_sourcelist, inactive_sourcelist);
	if (active_sourcelist.size() == 2) points++;
	testsrc3.Stop();
	DetermineActiveSources(active_sourcelist, inactive_sourcelist);
	if (active_sourcelist.size() == 1 && inactive_sourcelist.size() == 1) points++;
	testsrc2.Stop();
	DetermineActiveSources(active_sourcelist, inactive_sourcelist);
	if (active_sourcelist.size() == 0 && inactive_sourcelist.size() == 2) points++;
	DeleteSource(&testsrc3);
	if (sourcelist.size() == 1) points++;
	DeleteSource(&testsrc2);
	if (sourcelist.size() == 0) points++;
	mytest.SubTestComplete("Sound management testing", points, 6);

	points = 0;
	SOUNDSOURCE & testsrc4 = NewSource(testwav);
	active_sourcelist.clear();
	active_sourcelist.push_back(&testsrc4);
	testsrc4.Set3DEffects(false);
	testsrc4.SetGain(0.5);
	VERTEX spos;
	spos.Set(300,0,0);
	testsrc4.SetPosition(spos);
	VERTEX lpos;
	QUATERNION lrot;
	Compute3DEffects(active_sourcelist, lpos, lrot);
	if (testsrc4.ComputedGain(1) == 0.5 && testsrc4.ComputedGain(2) == 0.5) points++;
	testsrc4.Set3DEffects(true);
	Compute3DEffects(active_sourcelist, lpos, lrot);
	if (testsrc4.ComputedGain(2) == 0.0) points++;
	if (testsrc4.ComputedGain(1) < 0.1) points++;
	mytest.SubTestComplete("Sound effects testing", points, 3);

	{
		points = 0;
		SOUNDFILTER filt1;
		int size = 10;
		int data1[size];
		int data2[size];
		for (int i = 0; i < size; i++)
		{
			if (i < 3)
			{
				data1[i] = data2[i] = 0;
			}
			else
			{
				data1[i] = 1000;
				data2[i] = 0;
			}
		}
		float xc[2];
		float yc[2];
		xc[0] = 0.1; //simple low-pass filter
		xc[1] = 0;
		yc[0] = 0;
		yc[1] = 0.9;
		filt1.SetFilter(1, xc, yc);
		filt1.Filter(data1,data2,5);
		if (data1[3] == 100 && data2[3] == 0) points++;
		filt1.Filter(&(data1[5]),&(data2[5]),5);
		if (data1[6] == 342 && data1[9] == 519 && data2[9] == 0) points++;
		//cout << data1[6] << endl;
		//for (int i = 0; i < 10; i++) cout << i << ". " << data1[i] << endl;

		mytest.SubTestComplete("Sound filter testing", points, 2);
	}

	return mytest;
}*/

void SOUND_CallbackWrapper(void *soundclass, Uint8 *stream, int len)
{
	((SOUND*)soundclass)->Callback16bitstereo(soundclass, stream, len);
}

bool SOUND::Init(int buffersize, std::ostream & info_output, std::ostream & error_output)
{
	if (disable || initdone)
		return false;
	
	sourcelistlock = SDL_CreateMutex();

	SDL_AudioSpec desired, obtained;

	desired.freq = 44100;
	desired.format = AUDIO_S16SYS;
	desired.samples = buffersize;
	desired.callback = SOUND_CallbackWrapper;
	desired.userdata = this;
	desired.channels = 2;

	if (SDL_OpenAudio(&desired, &obtained) < 0)
	{
		//string error = SDL_GetError();
		//UNRECOVERABLE_ERROR_FUNCTION(__FILE__,__LINE__,"Error opening audio device.");
		error_output << "Error opening audio device, disabling sound." << endl;
		//throw EXCEPTION(__FILE__, __LINE__, "Error opening audio device: " + error);
		DisableAllSound();
		return false;
	}

	int frequency = obtained.freq;
	int channels = obtained.channels;
	int samples = obtained.samples;
	int bytespersample = 2;
	if (obtained.format == AUDIO_U8 || obtained.format == AUDIO_S8)
	{
		bytespersample = 1;
	}

	if (obtained.format != desired.format)
	{
		//cout << "Warning: obtained audio format isn't the same as the desired format!" << endl;
		error_output << "Obtained audio format isn't the same as the desired format, disabling sound." << endl;
		DisableAllSound();
		return false;
	}

	stringstream dout;
	dout << "Obtained audio device:" << endl;
	dout << "Frequency: " << frequency << endl;
	dout << "Format: " << obtained.format << endl;
	dout << "Bits per sample: " << bytespersample * 8 << endl;
	dout << "Channels: " << channels << endl;
	dout << "Silence: " << (int) obtained.silence << endl;
	dout << "Samples: " << samples << endl;
	dout << "Size: " << (int) obtained.size << endl;
	info_output << "Sound initialization information:" << endl << dout.str();
	//cout << dout.str() << endl;
	if (bytespersample != 2 || obtained.channels != desired.channels || obtained.freq != desired.freq)
	{
		//throw EXCEPTION(__FILE__, __LINE__, "Sound did not create a 44.1kHz, 16 bit, stereo device as requested.");
		//cerr << __FILE__ << "," << __LINE__ << ": Sound interface did not create a 44.1kHz, 16 bit, stereo device as requested.  Disabling game.sound." << endl;
		error_output << "Sound interface did not create a 44.1kHz, 16 bit, stereo device as requested.  Disabling sound." << endl;
		DisableAllSound();
		return false;
	}

	deviceinfo = SOUNDINFO(samples, frequency, channels, bytespersample);

	initdone = true;

	SetMasterVolume(1.0);

	return true;
}

void SOUND::Pause(const bool pause_on)
{
	if (paused == pause_on) //take no action if no change
		return;

	paused = pause_on;
	if (pause_on)
	{
		//cout << "sound pause on" << endl;
		SDL_PauseAudio(1);
	}
	else
	{
		//cout << "sound pause off" << endl;
		SDL_PauseAudio(0);
	}
}

void SOUND::Callback16bitstereo(void *myself, Uint8 *stream, int len)
{
	assert(this == myself);
	assert(initdone);

	list <SOUNDSOURCE *> active_sourcelist;
	list <SOUNDSOURCE *> inactive_sourcelist;
	
	LockSourceList();
	
	DetermineActiveSources(active_sourcelist, inactive_sourcelist);
	Compute3DEffects(active_sourcelist);//, cam.GetPosition().ScaleR(-1), cam.GetRotation());

	//increment inactive sources
	for (list <SOUNDSOURCE *>::iterator s = inactive_sourcelist.begin(); s != inactive_sourcelist.end(); s++)
	{
		(*s)->IncrementWithPitch(len/4);
	}

	int * buffer1 = new int[len/4];
	int * buffer2 = new int[len/4];
	for (list <SOUNDSOURCE *>::iterator s = active_sourcelist.begin(); s != active_sourcelist.end(); s++)
	{
		SOUNDSOURCE * src = *s;
		src->SampleAndAdvanceWithPitch16bit(buffer1, buffer2, len/4);
		for (int f = 0; f < src->NumFilters(); f++)
		{
			src->GetFilter(f).Filter(buffer1, buffer2, len/4);
		}
		volume_filter.Filter(buffer1, buffer2, len/4);
		if (s == active_sourcelist.begin())
		{
			for (int i = 0; i < len/4; i++)
			{
				int pos = i*2;
				((short *) stream)[pos] = (buffer1[i]);
				((short *) stream)[pos+1] = (buffer2[i]);
			}
		}
		else
		{
			for (int i = 0; i < len/4; i++)
			{
				int pos = i*2;
				((short *) stream)[pos] += (buffer1[i]);
				((short *) stream)[pos+1] += (buffer2[i]);
			}
		}
	}
	delete [] buffer1;
	delete [] buffer2,
	
	UnlockSourceList();

	//cout << active_sourcelist.size() << "," << inactive_sourcelist.size() << endl;

	if (active_sourcelist.empty())
	{
		for (int i = 0; i < len/4; i++)
		{
			int pos = i*2;
			((short *) stream)[pos] = ((short *) stream)[pos+1] = 0;
		}
	}

	CollectGarbage();

	//cout << "Callback: " << len << endl;
}

SOUND::~SOUND()
{
	if (initdone)
	{
		SDL_CloseAudio();
	}
	
	if (sourcelistlock)
		SDL_DestroyMutex(sourcelistlock);
}
/*
#define PREFER_BRANCHING_OVER_MULTIPLY
#define FASTER_SOUNDBUFFER_ACCESS
#define FAST_SOUNDBUFFER_ACCESS

inline void SOUNDSOURCE::SampleAndAdvance16bit(int * chan1, int * chan2, int len)
{
	assert(buffer);
#ifdef FAST_SOUNDBUFFER_ACCESS
	//if (buffer->info.channels == 2)
	{
		unsigned int samples = buffer->info.GetSamples();

#ifdef FASTER_SOUNDBUFFER_ACCESS
		unsigned int remaining_len = len;
		unsigned int this_start = 0;
		while (remaining_len > 0)
		{
			unsigned int this_run = remaining_len;
			unsigned int this_end = this_start+this_run;

			if (playing)
			{
				if (sample_pos + remaining_len > samples)
				{
					this_run = samples - sample_pos;
					this_end = this_start+this_run;
				}

				if (buffer->info.GetChannels() == 2)
				{
					for (unsigned int i = this_start, n = sample_pos*2; i < this_end; i++,n+=2)
					{
						chan1[i] = ((short *)buffer->sound_buffer)[n];
						chan2[i] = ((short *)buffer->sound_buffer)[n+1];
						//#endif
					}
				}
				else //it is assumed that this means channels == 1
				{
					for (unsigned int i = this_start, n = sample_pos; i < this_end; i++, n++)
					{
						chan1[i] = chan2[i] = ((short *)buffer->sound_buffer)[n];
						//#endif
					}
				}
			}
			else
			{
				//cout << "Finishing run of " << this_run << endl;
				for (unsigned int i = this_start; i < this_end; i++)
				{
					chan1[i] = chan2[i] = 0;
				}
			}

			unsigned int newpos = sample_pos + this_run;
			if (newpos >= samples && !loop)
			{
				playing = 0;
				//cout << "Stopping play" << endl;
			}
			else
			{
				sample_pos = (newpos) % samples;
				//cout << newpos << "<" << samples << " loop: " << loop << endl;
			}

			//cout << remaining_len << " - " << this_run << endl;
			remaining_len -= this_run;
			this_start += this_run;
		}

		//cout << remaining_len << endl;
#else
		if (buffer->info.channels == 2)
		{
			for (int i = 0; i < len; i++)
			{
				if (playing)
				{
					int sample_idx = sample_pos*2;
					chan1[i] = ((short *)buffer->sound_buffer)[sample_idx];
					chan2[i] = ((short *)buffer->sound_buffer)[sample_idx+1];
					//#endif

					unsigned int newpos = sample_pos + 1;
					if (newpos >= samples && !loop)
						playing = 0;
					else
						sample_pos = (newpos) % samples;
				}
				else
				{
					chan1[i] = chan2[i] = 0;
				}
			}
		}
		else //channels == 1
		{
			for (int i = 0; i < len; i++)
			{
				if (playing)
				{
					chan1[i] = ((short *)buffer->sound_buffer)[sample_pos];
					chan2[i] = ((short *)buffer->sound_buffer)[sample_pos];
					//#endif

					int newpos = sample_pos + 1;
					if (!loop && newpos >= samples && playing)
						playing = 0;
					else
						sample_pos = (newpos) % samples;
				}
				else
				{
					chan1[i] = chan2[i] = 0;
				}
			}
		}
#endif
	}
#else
	for (int i = 0; i < len; i++)
	{
#ifdef PREFER_BRANCHING_OVER_MULTIPLY
		if (playing)
		{
			chan1[i] = (int) (buffer->GetSample16bit(1, sample_pos)*computed_gain1);
			chan2[i] = (int) (buffer->GetSample16bit(2, sample_pos)*computed_gain2);

			Increment(1);
		}
		else
		{
			chan1[i] = chan2[i] = 0;
		}
#else
		chan1[i] = (int) (buffer->GetSample16bit(1, sample_pos)*playing*computed_gain1);
		chan2[i] = (int) (buffer->GetSample16bit(2, sample_pos)*playing*computed_gain2);

		//stop playing if at the end of loop and not looping
		//this line is a complicated piece of engineering to avoid branching...
		//consider that (sample_pos + 1) / buffer.GetPtr()->GetSoundInfo().GetSamples() is 1 if we're at the end, 0 if we're not
		//let e = (sample_pos + 1) / buffer.GetPtr()->GetSoundInfo().GetSamples()
		//let l = loop
		//let p = playing
		//X means "don't care"
		//we want:
		//e=0 && l=X => p=1
		//e=1 && l=1 => p=1
		//e=1 && l=0 => p=0
		playing = 1 - ((sample_pos + 1) / samples)*(1-loop);

		sample_pos = (sample_pos + 1*playing) % samples;
#endif
	}
#endif
}

inline void SOUNDSOURCE::Increment(int num)
{
	int samples = buffer->GetSoundInfo().GetSamples()/buffer->GetSoundInfo().GetChannels();

	int newpos = sample_pos + num;
	if (newpos >= samples && !loop)
		playing = 0;
	else
		sample_pos = (newpos) % samples;
}

//#define RESAMPLE_QUALITY_LOWEST
//#define RESAMPLE_QUALITY_MEDIUM
//#define RESAMPLE_QUALITY_FAST_MEDIUM
#define RESAMPLE_QUALITY_FASTER_MEDIUM
//#define RESAMPLE_QUALITY_HIGH*/

///output buffers chan1 (left) and chan2 (right) will be filled with "len" 16-bit samples
void SOUNDSOURCE::SampleAndAdvanceWithPitch16bit(int * chan1, int * chan2, int len)
{
	assert(buffer);
	int samples = buffer->info.GetSamples(); //the number of 16-bit samples in the buffer with left and right channels SUMMED (so for stereo signals the number of samples per channel is samples/2)

	float n_remain = sample_pos_remainder; //the fractional portion of the current playback position for this soundsource
	int n = sample_pos; //the integer portion of the current playback position for this soundsource PER CHANNEL (i.e., this will range from 0 to samples/2)

	float samplecount = 0; //floating point record of how far the playback position has increased during this callback

	const int chan = buffer->info.GetChannels();

	samples -= samples % chan; //correct the number of samples in odd situations where we have stereo channels but an odd number of channels

	const int samples_per_channel = samples / chan; //how many 16-bit samples are in a channel of audio

	assert((int)sample_pos <= samples_per_channel);

	last_pitch = pitch; //this bypasses pitch rate limiting, because it isn't a very useful effect, turns out.

	if (playing)
	{
		assert(len > 0);

		int samp1[2]; //sample(s) to the left of the floating point sample position
		int samp2[2]; //sample(s) to the right of the floating point sample position

		int idx = n*chan; //the integer portion of the current 16-bit playback position in the input buffer, accounting for duplication of samples for stereo waveforms

		const float maxrate = 1.0/(44100.0*0.01); //the maximum allowed rate of gain change per sample
		const float negmaxrate = -maxrate;
		//const float maxpitchrate = maxrate; //the maximum allowed rate of pitch change per sample
		//const float negmaxpitchrate = -maxpitchrate;
		int16_t * buf = (int16_t *)buffer->sound_buffer; //access to the 16-bit sound input buffer
		const int chaninc = chan - 1; //the offset to use when fetching the second channel from the sound input buffer (yes, this will be zero for a mono sound buffer)

		float gaindiff1(0), gaindiff2(0);//, pitchdiff(0); //allocate memory here so we don't have to in the loop

		for (int i = 0; i  < len; ++i)
		{
			//do gain change rate limiting
			gaindiff1 = computed_gain1 - last_computed_gain1;
			gaindiff2 = computed_gain2 - last_computed_gain2;
			if (gaindiff1 > maxrate)
				gaindiff1 = maxrate;
			else if (gaindiff1 < negmaxrate)
				gaindiff1 = negmaxrate;
			if (gaindiff2 > maxrate)
				gaindiff2 = maxrate;
			else if (gaindiff2 < negmaxrate)
				gaindiff2 = negmaxrate;
			last_computed_gain1 = last_computed_gain1 + gaindiff1;
			last_computed_gain2 = last_computed_gain2 + gaindiff2;

			//do pitch change rate limiting
			/*pitchdiff = pitch - last_pitch;
			if (pitchdiff > maxpitchrate)
				pitchdiff = maxpitchrate;
			else if (pitchdiff < negmaxpitchrate)
				pitchdiff = negmaxpitchrate;
			last_pitch = last_pitch + pitchdiff;*/

			if (n >= samples_per_channel && !loop) //end playback if we've finished playing the buffer and looping is not enabled
			{
				//stop playback
				chan1[i] = chan2[i] = 0;
				playing = 0;
			}
			else //if not at the end of a non-looping sample, or if the sample is looping
			{
				idx = chan*(n % samples_per_channel); //recompute the buffer position accounting for looping

				assert(idx+chaninc < samples); //make sure we don't read past the end of the buffer

				samp1[0] = buf[idx]; //the sample to the left of the playback position, channel 0
				samp1[1] = buf[idx+chaninc]; //the sample to the left of the playback position, channel 1

				idx = (idx + chan) % samples;

				assert(idx+chaninc < samples); //make sure we don't read past the end of the buffer

				samp2[0] = buf[idx]; //the sample to the right of the playback position, channel 0
				samp2[1] = buf[idx+chaninc]; //the sample to the right of the playback position, channel 1

				//samp2[0] = samp1[0]; //disable interpolation, for debug purposes
				//samp2[1] = samp1[1];

				chan1[i] = (int) ((n_remain*(samp2[0] - samp1[0]) + samp1[0])*last_computed_gain1); //set the output buffer to the linear interpolation between the left and right samples for channel 0
				chan2[i] = (int) ((n_remain*(samp2[1] - samp1[1]) + samp1[1])*last_computed_gain2); //set the output buffer to the linear interpolation between the left and right samples for channel 1

				n_remain += last_pitch; //increment the playback position
				const unsigned int ninc = (unsigned int) n_remain;
				n += ninc; //update the integer portion of the playback position
				n_remain -= (float) ninc; //update the fractional portion of the playback position

				samplecount += last_pitch; //increment the playback delta position counter.  this will eventually be added to the soundsource playback position variables.
			}
		}

		double newpos = sample_pos + sample_pos_remainder + samplecount; //calculate a floating point new playback position based on where we started plus how many samples we just played

		if (newpos >= samples_per_channel && !loop) //end playback if we've finished playing the buffer and looping is not enabled
			playing = 0;
		else //if not at the end of a non-looping sample, or if the sample is looping
		{
			while (newpos >= samples_per_channel) //this while loop is like a floating point modulo operation that sets newpos = newpos % samples
				newpos -= samples_per_channel;
			sample_pos = (unsigned int) newpos; //save the integer portion of the current playback position back to the soundsource
			sample_pos_remainder = newpos - sample_pos; //save the fractional portion of the current playback position back to the soundsource
		}
		
		if (playing)
			assert((int)sample_pos <= samples_per_channel);
		//assert(0);
	}
	else //if not playing
	{
		for (int i = 0; i  < len; ++i)
		{
			chan1[i] = chan2[i] = 0; //fill output buffers with silence
		}
	}
}


inline void SOUNDSOURCE::IncrementWithPitch(int num)
{
	int samples = buffer->GetSoundInfo().GetSamples()/buffer->GetSoundInfo().GetChannels();
	double newpos = sample_pos + sample_pos_remainder + (num)*pitch;
	if (newpos >= samples && !loop)
		playing = 0;
	else
	{
		while (newpos >= samples)
			newpos -= samples;
		sample_pos = (unsigned int) newpos;
		sample_pos_remainder = newpos - sample_pos;
	}
}

/*inline void SOUNDSOURCE::Sample16bit(unsigned int peekoffset, int & chan1, int & chan2)
{
	//lines below are used for performance testing to find bottlenecks
	//chan1 = 123;
	//chan2 = 123;
	//chan1 = buffer->GetSample16bit(1, peekoffset);
	//chan2 = buffer->GetSample16bit(2, peekoffset);

	int samples = buffer->GetSoundInfo().GetSamples();

#ifdef PREFER_BRANCHING_OVER_MULTIPLY
	if (playing)
	{
		int newsamplepos = (sample_pos + peekoffset);
		if (newsamplepos < samples)
		{
			chan1 = (int) (buffer->GetSample16bit(1, newsamplepos)*computed_gain1);
			chan2 = (int) (buffer->GetSample16bit(2, newsamplepos)*computed_gain2);
		}
		else if (loop)
		{
			int peekpos = (sample_pos + peekoffset) % samples;
			chan1 = (int) (buffer->GetSample16bit(1, peekpos)*computed_gain1);
			chan2 = (int) (buffer->GetSample16bit(2, peekpos)*computed_gain2);
		}
		else
		{
			chan1 = chan2 = 0;
		}
	}
	else
	{
		chan1 = chan2 = 0;
	}
#else
	int peekplaying = 1 - ((sample_pos + peekoffset) / samples)*(1-loop);
	peekplaying *= playing;
	int peekpos = (sample_pos + peekoffset*peekplaying) % samples;
	chan1 = (int) (buffer->GetSample16bit(1, peekpos)*computed_gain1*peekplaying);
	chan2 = (int) (buffer->GetSample16bit(2, peekpos)*computed_gain2*peekplaying);
#endif
}

inline void SOUNDSOURCE::Advance(unsigned int offset)
{
	playing = 1 - ((sample_pos + offset) / buffer->GetSoundInfo().GetSamples())*(1-loop);

	sample_pos = (sample_pos + offset*playing) % buffer->GetSoundInfo().GetSamples();
}*/

void SOUND::CollectGarbage()
{
	if (disable)
		return;

	list <SOUNDSOURCE *> todel;
	for (list <SOUNDSOURCE *>::iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if (!(*i)->Audible() && (*i)->GetAutoDelete())
		{
			todel.push_back(*i);
		}
	}

	for (list <SOUNDSOURCE *>::iterator i = todel.begin(); i != todel.end(); ++i)
	{
		RemoveSource(*i);
	}

	//cout << sourcelist.size() << endl;
}

void SOUND::DetermineActiveSources(list <SOUNDSOURCE *> & active_sourcelist, list <SOUNDSOURCE *> & inaudible_sourcelist) const
{
	active_sourcelist.clear();
	inaudible_sourcelist.clear();
	//int sourcenum = 0;
	for (list <SOUNDSOURCE *>::const_iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if ((*i)->Audible())
		{
			active_sourcelist.push_back(*i);
			//cout << "Tick: " << &(*i) << endl;
			//cout << "Source is audible: " << i->GetName() << ", " << i->GetGain() << ":" << i->ComputedGain(1) << "," << i->ComputedGain(2) << endl;
			//cout << "Source " << sourcenum << " is audible: " << i->GetName() << endl;
		}
		else
		{
			inaudible_sourcelist.push_back(*i);
		}
		//sourcenum++;
	}
	//cout << "sounds active: " << active_sourcelist.size() << ", sounds inactive: " << inaudible_sourcelist.size() << endl;
}

void SOUND::RemoveSource(SOUNDSOURCE * todel)
{
	if (disable)
		return;

	assert(todel);

	list <SOUNDSOURCE *>::iterator delit = sourcelist.end();
	for (list <SOUNDSOURCE *>::iterator i = sourcelist.begin(); i != sourcelist.end(); ++i)
	{
		if (*i == todel)
			delit = i;
	}

	//assert(delit != sourcelist.end()); //can't find source to delete //update: don't assert, just do a check

	LockSourceList();
	if (delit != sourcelist.end())
		sourcelist.erase(delit);
	UnlockSourceList();
}

/*void SOUND::NewSourcePlayOnce(const string & buffername)
{
	if (disable)
		return;

	SOUNDSOURCE & src = NewSource(buffername);
	src.SetAutoDelete(true);
	src.Set3DEffects(false);
	src.Play();
}*/

void SOUND::Compute3DEffects(list <SOUNDSOURCE *> & sources, const MATHVECTOR <float, 3> & listener_pos, const QUATERNION <float> & listener_rot) const
{
	for (list <SOUNDSOURCE *>::iterator i = sources.begin(); i != sources.end(); ++i)
	{
		if ((*i)->Get3DEffects())
		{
			MATHVECTOR <float, 3> relvec = (*i)->GetPosition() - listener_pos;
			//std::cout << "sound pos: " << (*i)->GetPosition() << endl;;
			//std::cout << "listener pos: " << listener_pos << endl;;
			//cout << "listener pos: ";listener_pos.DebugPrint();
			//cout << "camera pos: ";cam.GetPosition().ScaleR(-1.0).DebugPrint();
			float len = relvec.Magnitude();
			if (len < 0.1)
			{
				relvec[2] = 0.1;
				len = relvec.Magnitude();
			}
			listener_rot.RotateVector(relvec);
			float cgain = log(1000.0 / pow((double)len, 1.3)) / log(100.0);
			if (cgain > 1.0)
				cgain = 1.0;
			if (cgain < 0.0)
				cgain = 0.0;
			float xcoord = -relvec.Normalize()[1];
			//std::cout << (*i)->GetPosition() << " || " << listener_pos << " || " << xcoord << endl;
			float pgain1 = -xcoord;
			if (pgain1 < 0)
				pgain1 = 0;
			float pgain2 = xcoord;
			if (pgain2 < 0)
				pgain2 = 0;
			//cout << cgain << endl;
			//cout << xcoord << endl;
			(*i)->SetComputationResults(cgain*(*i)->GetGain()*(1.0-pgain1), cgain*(*i)->GetGain()*(1.0-pgain2));
		}
		else
		{
			(*i)->SetComputationResults((*i)->GetGain(), (*i)->GetGain());
		}
	}
}

bool SOUNDSOURCE::Audible() const
{
	bool canhear = (playing == 1) && (GetGain() > 0);

	return canhear;
}

void SOUNDSOURCE::SetGainSmooth(const float newgain, const float dt)
{
	//float coeff = dt*40.0;
	if (dt <= 0)
		return;

	//low pass filter

	//rate limit
	float ndt = dt * 4.0;
	float delta = newgain - gain;
	if (delta > ndt)
		delta = ndt;
	if (delta < -ndt)
		delta = -ndt;
	gain = gain + delta;
}

void SOUNDSOURCE::SetPitchSmooth(const float newpitch, const float dt)
{
	//float coeff = dt*40.0;
	if (dt > 0)
	{
		//low pass filter

		//rate limit
		float ndt = dt * 4.0;
		float delta = newpitch - pitch;
		if (delta > ndt)
			delta = ndt;
		if (delta < -ndt)
			delta = -ndt;
		pitch = pitch + delta;
	}
}

//the coefficients are arrays of size xcoeff[neworder+1]
void SOUNDFILTER::SetFilter(const int neworder, const float * xcoeff, const float * ycoeff)
{
	assert(!(neworder > MAX_FILTER_ORDER || neworder < 0));
		//cerr << __FILE__ << "," << __LINE__ << "Filter order is larger than maximum order" << endl;
		//UNRECOVERABLE_ERROR_FUNCTION(__FILE__,__LINE__,"Filter order is larger than maximum order");

	order = neworder;
	for (int i = 0; i < neworder+1; i++)
	{
		xc[i] = xcoeff[i];
		yc[i] = ycoeff[i];
	}
}

void SOUNDFILTER::Filter(int * chan1, int * chan2, const int len)
{
	for (int i = 0; i < len; i++)
	{
		//store old state
		for (int s = 1; s <= order; s++)
		{
			statex[0][s] = statex[0][s-1];
			statex[1][s] = statex[1][s-1];

			statey[0][s] = statey[0][s-1];
			statey[1][s] = statey[1][s-1];
		}

		//set the sample state for now to the current input
		statex[0][0] = chan1[i];
		statex[1][0] = chan2[i];

		switch (order)
		{
			case 1:
				chan1[i] = (int) (statex[0][0]*xc[0]+statex[0][1]*xc[1]+statey[0][1]*yc[1]);
				chan2[i] = (int) (statex[1][0]*xc[0]+statex[1][1]*xc[1]+statey[1][1]*yc[1]);
				break;
			case 0:
				chan1[i] = (int) (statex[0][0]*xc[0]);
				chan2[i] = (int) (statex[1][0]*xc[0]);
				break;
			default:
				break;
		}

		//store the state of the output
		statey[0][0] = chan1[i];
		statey[1][0] = chan2[i];
	}
}

SOUNDFILTER & SOUNDSOURCE::GetFilter(int num)
{
	int curnum = 0;
	for (list <SOUNDFILTER>::iterator i = filters.begin(); i != filters.end(); ++i)
	{
		if (num == curnum)
			return *i;
		curnum++;
	}

	//cerr << __FILE__ << "," << __LINE__ << "Asked for a non-existant filter" << endl;
	//UNRECOVERABLE_ERROR_FUNCTION(__FILE__,__LINE__,"Asked for a non-existant filter");
	assert(0);
	SOUNDFILTER * nullfilt = NULL;
	return *nullfilt;
}

void SOUNDFILTER::SetFilterOrder1(float xc0, float xc1, float yc1)
{
	order = 1;
	xc[0] = xc0;
	xc[1] = xc1;
	yc[1] = yc1;
}

void SOUNDFILTER::SetFilterOrder0(float xc0)
{
	order = 0;
	xc[0] = xc0;
}

void SOUND::LockSourceList()
{
	if(SDL_mutexP(sourcelistlock)==-1){
		assert(0 && "Couldn't lock mutex");
	}
}

void SOUND::UnlockSourceList()
{
	if(SDL_mutexV(sourcelistlock)==-1){
		assert(0 && "Couldn't unlock mutex");
	}
}

