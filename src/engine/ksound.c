/***************************************************************************
 ksound.c Copyright (C) 1999 Christoph Reichenbach


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/

#include <engine.h>
#include <sfx_player.h>

#define _K_SCI0_SOUND_INIT_HANDLE 0
#define _K_SCI0_SOUND_PLAY_HANDLE 1
#define _K_SCI0_SOUND_NOP 2
#define _K_SCI0_SOUND_DISPOSE_HANDLE 3
#define _K_SCI0_SOUND_MUTE_SOUND 4
#define _K_SCI0_SOUND_STOP_HANDLE 5
#define _K_SCI0_SOUND_SUSPEND_HANDLE 6
#define _K_SCI0_SOUND_RESUME_HANDLE 7
#define _K_SCI0_SOUND_VOLUME 8
#define _K_SCI0_SOUND_UPDATE_VOL_PRI 9
#define _K_SCI0_SOUND_FADE_HANDLE 10
#define _K_SCI0_SOUND_GET_POLYPHONY 11
#define _K_SCI0_SOUND_STOP_ALL 12

#define _K_SCI01_SOUND_MASTER_VOLME 0 /* Set/Get */
#define _K_SCI01_SOUND_MUTE_SOUND 1
#define _K_SCI01_SOUND_UNUSED 2
#define _K_SCI01_SOUND_GET_POLYPHONY 3
#define _K_SCI01_SOUND_UPDATE_HANDLE 4
#define _K_SCI01_SOUND_INIT_HANDLE 5
#define _K_SCI01_SOUND_DISPOSE_HANDLE 6
#define _K_SCI01_SOUND_PLAY_HANDLE 7
#define _K_SCI01_SOUND_STOP_HANDLE 8
#define _K_SCI01_SOUND_SUSPEND_HANDLE 9 /* or resume */
#define _K_SCI01_SOUND_FADE_HANDLE 10
#define _K_SCI01_SOUND_UPDATE_CUES 11
#define _K_SCI01_SOUND_PITCH_WHEEL 12
#define _K_SCI01_SOUND_REVERB 13 /* Get/Set */
#define _K_SCI01_SOUND_CONTROLLER 14
 
#define _K_SCI1_SOUND_MASTER_VOLME 0 /* Set/Get */
#define _K_SCI1_SOUND_MUTE_SOUND 1
#define _K_SCI1_SOUND_UNUSED1 2
#define _K_SCI1_SOUND_GET_POLYPHONY 3
#define _K_SCI1_SOUND_CD_AUDIO 4
#define _K_SCI1_SOUND_PLAY_HANDLE 5
#define _K_SCI1_SOUND_INIT_HANDLE 6
#define _K_SCI1_SOUND_DISPOSE_HANDLE 7
#define _K_SCI1_SOUND_UPDATE_HANDLE 8
#define _K_SCI1_SOUND_STOP_HANDLE 9
#define _K_SCI1_SOUND_SUSPEND_HANDLE 10 /* or resume */
#define _K_SCI1_SOUND_FADE_HANDLE 11
#define _K_SCI1_SOUND_HOLD_HANDLE 12
#define _K_SCI1_SOUND_UNUSED2 13
#define _K_SCI1_SOUND_SET_HANDLE_VOLUME 14
#define _K_SCI1_SOUND_SET_HANDLE_PRIORITY 15
#define _K_SCI1_SOUND_SET_HANDLE_LOOP 16
#define _K_SCI1_SOUND_UPDATE_CUES 17
#define _K_SCI1_SOUND_MIDI_SEND 18 
#define _K_SCI1_SOUND_REVERB 19 /* Get/Set */
#define _K_SCI1_SOUND_UPDATE_VOL_PRI 20

#define FROBNICATE_HANDLE(reg) ((reg).segment << 16 | (reg).offset)
#define DEFROBNICATE_HANDLE(handle) (make_reg((handle >> 16) & 0xffff, handle & 0xffff))


static song_iterator_t *
build_iterator(state_t *s, int song_nr, int type)
{
	resource_t *song = scir_find_resource(s->resmgr, sci_sound, song_nr, 0);

	if (!song)
		return NULL;

	return songit_new(song->data, song->size, type);
}




void
process_sound_events(state_t *s) /* Get all sound events, apply their changes to the heap */
{
	int result;
	song_handle_t handle;
	int cue;

	while ((result = sfx_poll(&s->sound, &handle, &cue))) {
		reg_t obj = DEFROBNICATE_HANDLE(handle);
		if (!is_object(s, obj)) {
			SCIkdebug(SCIkWARNING, "Non-object "PREG" received sound signal (%d/%d)\n", PRINT_REG(obj), result, cue);
			return;
		}

		switch (result) {

		case SI_LOOP:
			SCIkdebug(SCIkSOUND, "[process-sound] Song "PREG" looped (to %d)\n",
				  PRINT_REG(obj), cue);
			PUT_SEL32V(obj, signal, -1);
			break;

		case SI_CUE:
			SCIkdebug(SCIkSOUND, "[process-sound] Song "PREG" received cue %d\n",
				  PRINT_REG(obj), cue);
			PUT_SEL32V(obj, signal, cue);
			break;

		case SI_FINISHED:
			SCIkdebug(SCIkSOUND, "[process-sound] Song "PREG" finished\n",
				  PRINT_REG(obj));
			PUT_SEL32V(obj, signal, -1);
			PUT_SEL32V(obj, state, _K_SOUND_STATUS_STOPPED);
			break;

		default:
			sciprintf("Unexpected result from sfx_poll: %d\n", result);
			break;
		}
	}
}


reg_t
kDoSound_SCI0(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	reg_t obj = KP_ALT(1, NULL_REG);
	word command = UKPV(0);
	song_handle_t handle = FROBNICATE_HANDLE(obj);

	if (s->debug_mode & (1 << SCIkSOUNDCHK_NR)) {
		int i;

		SCIkdebug(SCIkSOUND, "Command 0x%x", command);
		switch (command) {
		case 0: sciprintf("[InitObj]"); break;
		case 1: sciprintf("[Play]"); break;
		case 2: sciprintf("[NOP]"); break;
		case 3: sciprintf("[DisposeHandle]"); break;
		case 4: sciprintf("[SetSoundOn(?)]"); break;
		case 5: sciprintf("[Stop]"); break;
		case 6: sciprintf("[Suspend]"); break;
		case 7: sciprintf("[Resume]"); break;
		case 8: sciprintf("[Get(Set?)Volume]"); break;
		case 9: sciprintf("[Signal: Obj changed]"); break;
		case 10: sciprintf("[Fade(?)]"); break;
		case 11: sciprintf("[ChkDriver]"); break;
		case 12: sciprintf("[StopAll]"); break;
		default: sciprintf("[unknown]"); break;
		}

		sciprintf("(");
		for (i = 1; i < argc; i++) {
			sciprintf(PREG, PRINT_REG(argv[i]));
			if (i + 1 < argc)
				sciprintf(", ");
		}
		sciprintf(")\n");
	}


	switch (command) {
	case _K_SCI0_SOUND_INIT_HANDLE:
		if (obj.segment) {
			sfx_add_song(&s->sound,
				     build_iterator(s, GET_SEL32V(obj, number),
						    SCI_SONG_ITERATOR_TYPE_SCI0),
				     0, handle);
			PUT_SEL32V(obj, state, _K_SOUND_STATUS_INITIALIZED);
		}
		break;

	case _K_SCI0_SOUND_PLAY_HANDLE:
		if (obj.segment) {
			sfx_song_set_status(&s->sound,
					    handle, SOUND_STATUS_PLAYING);
			sfx_song_set_loops(&s->sound,
					   handle, GET_SEL32V(obj, loop));
			PUT_SEL32V(obj, state, _K_SOUND_STATUS_PLAYING);
		}
		break;

	case _K_SCI0_SOUND_NOP:
		break;

	case _K_SCI0_SOUND_DISPOSE_HANDLE:
		if (obj.segment) {
			sfx_remove_song(&s->sound, handle);
		}
		break;

	case _K_SCI0_SOUND_STOP_HANDLE:
		if (obj.segment) {
			sfx_song_set_status(&s->sound,
					    handle, SOUND_STATUS_STOPPED);
			PUT_SEL32V(obj, state, SOUND_STATUS_STOPPED);
		}
		break;

	case _K_SCI0_SOUND_SUSPEND_HANDLE:
		if (obj.segment) {
			sfx_song_set_status(&s->sound,
					    handle, SOUND_STATUS_SUSPENDED);
			PUT_SEL32V(obj, state, SOUND_STATUS_SUSPENDED);
		}
		break;

	case _K_SCI0_SOUND_RESUME_HANDLE:
		if (obj.segment) {
			sfx_song_set_status(&s->sound,
					    handle, SOUND_STATUS_PLAYING);
			PUT_SEL32V(obj, state, SOUND_STATUS_PLAYING);
		}
		break;

	case _K_SCI0_SOUND_MUTE_SOUND: {
		/* if there's a parameter, we're setting it.  Otherwise,
		   we're querying it. */
		/*int param = UPARAM_OR_ALT(1,-1);

		if (param != -1)
		s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_MUTE, 0, param);
		else
		s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_MUTE, 0, 0);*/

	}
		break;

	case _K_SCI0_SOUND_VOLUME: {
		/* range from 0x0 to 0xf */
		/* parameter optional. If present, set.*/
		int vol = SKPV_OR_ALT(1, -1);

		if (vol != -1)
			sfx_set_volume(&s->sound, vol << 0xf);
		else
			s->r_acc = make_reg(0, sfx_get_volume(&s->sound) >> 0xf);
	}
		break;

	case _K_SCI0_SOUND_UPDATE_VOL_PRI:
		if (obj.segment) {
			sfx_song_set_loops(&s->sound,
					   handle, GET_SEL32V(obj, loop));
			sfx_song_renice(&s->sound,
					handle, GET_SEL32V(obj, priority));
		}
		break;

	case _K_SCI0_SOUND_FADE_HANDLE:
		/*s->sound_server->command(s, SOUND_COMMAND_FADE_HANDLE, obj, 120);*/ /* Fade out in 2 secs */
		break;

	case _K_SCI0_SOUND_GET_POLYPHONY:
		s->r_acc = make_reg(0, sfx_get_player_polyphony());
		break;

	case _K_SCI0_SOUND_STOP_ALL:
		sfx_all_stop(&s->sound);
		break;

	default:
		SCIkwarn(SCIkWARNING, "Unhandled DoSound command: %x\n", command);

	}
	process_sound_events(s); /* Take care of incoming events */

	return s->r_acc;
}


reg_t
kDoSound_SCI01(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	word command = UKPV(0);
	reg_t obj = KP_ALT(1, NULL_REG);
	song_handle_t handle = FROBNICATE_HANDLE(obj);

	if (s->debug_mode & (1 << SCIkSOUNDCHK_NR)) {
		int i;

		SCIkdebug(SCIkSOUND, "Command 0x%x", command);
		switch (command) {
		case 0: sciprintf("[MasterVolume]"); break;
		case 1: sciprintf("[Mute]"); break;
		case 2: sciprintf("[NOP(2)]"); break;
		case 3: sciprintf("[GetPolyphony]"); break;
		case 4: sciprintf("[Update]"); break;
		case 5: sciprintf("[Init]"); break;
		case 6: sciprintf("[Dispose]"); break;
		case 7: sciprintf("[Play]"); break;
		case 8: sciprintf("[Stop]"); break;
		case 9: sciprintf("[Suspend]"); break;
		case 10: sciprintf("[Fade]"); break;
		case 11: sciprintf("[UpdateCues]"); break;
		case 12: sciprintf("[PitchWheel]"); break;
		case 13: sciprintf("[Reverb]"); break;
		case 14: sciprintf("[Controller]"); break;
		default: sciprintf("[unknown]"); break;
		}

		sciprintf("(");
		for (i = 1; i < argc; i++) {
			sciprintf(PREG, PRINT_REG(argv[i]));
			if (i + 1 < argc)
				sciprintf(", ");
		}
		sciprintf(")\n");
	}

	switch (command)
	{
	case _K_SCI01_SOUND_MASTER_VOLME :
	{
		int vol = SKPV_OR_ALT(1, -1);

		if (vol != -1)
			sfx_set_volume(&s->sound, vol << 0xf);
		else
			s->r_acc = make_reg(0, sfx_get_volume(&s->sound) >> 0xf);
		break;
	}
	case _K_SCI01_SOUND_MUTE_SOUND :
	{
		/* if there's a parameter, we're setting it.  Otherwise,
		   we're querying it. */
		/*int param = UPARAM_OR_ALT(1,-1);

		if (param != -1)
			s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_MUTE, 0, param);
		else
		s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_MUTE, 0, 0);*/

		break;
	}
	case _K_SCI01_SOUND_UNUSED :
	{
		break;
	}
	case _K_SCI01_SOUND_GET_POLYPHONY :
	{
		s->r_acc = make_reg(0, sfx_get_player_polyphony());
		break;
	}
	case _K_SCI01_SOUND_PLAY_HANDLE :
	{

		int looping = GET_SEL32V(obj, loop);
		int vol = GET_SEL32V(obj, vol);
		int pri = GET_SEL32V(obj, pri);

		if (obj.segment) {
			sfx_song_set_status(&s->sound,
					    handle, SOUND_STATUS_PLAYING);
			sfx_song_set_loops(&s->sound,
					   handle, looping);
			PUT_SEL32V(obj, state, _K_SOUND_STATUS_PLAYING);
			sfx_song_renice(&s->sound,
					handle, pri);
		}

		break;
	}
	case _K_SCI01_SOUND_INIT_HANDLE :
	{
		int number = GET_SEL32V(obj, number);

		int looping = GET_SEL32V(obj, loop);
		int vol = GET_SEL32V(obj, vol);
		int pri = GET_SEL32V(obj, pri);

		if (obj.segment) {
			sfx_add_song(&s->sound,
				     build_iterator(s, GET_SEL32V(obj, number),
						    SCI_SONG_ITERATOR_TYPE_SCI1),
				     0, handle);
			PUT_SEL32V(obj, state, _K_SOUND_STATUS_INITIALIZED);
		}
		break;
	}
	case _K_SCI01_SOUND_DISPOSE_HANDLE :
	{
		if (obj.segment) {
			sfx_remove_song(&s->sound, handle);
		}
		break;
	}
	case _K_SCI01_SOUND_UPDATE_HANDLE :
	{
		/* FIXME: Get these from the sound server */
		int signal = 0;
		int min = 0;
		int sec = 0;
		int frame = 0;

		/* FIXME: Update the sound server state with 'vol' */
		int looping = GET_SEL32V(obj, loop);
		int vol = GET_SEL32V(obj, vol);
		int pri = GET_SEL32V(obj, pri);

		sfx_song_set_loops(&s->sound,
				   handle, looping);
		sfx_song_renice(&s->sound,
				handle, pri);

		PUT_SEL32V(obj, signal, signal);
		PUT_SEL32V(obj, min, min);
		PUT_SEL32V(obj, sec, sec);
		PUT_SEL32V(obj, frame, frame);

		break;
	}
	case _K_SCI01_SOUND_STOP_HANDLE :
	{
		PUT_SEL32V(obj, signal, -1);
		if (obj.segment) {
			sfx_song_set_status(&s->sound,
					    handle, SOUND_STATUS_STOPPED);
			PUT_SEL32V(obj, state, SOUND_STATUS_STOPPED);
		}
		break;
	}
	case _K_SCI01_SOUND_SUSPEND_HANDLE :
	{
		int state = UKPV(2);
		int setstate = (state)?
			SOUND_STATUS_SUSPENDED : SOUND_STATUS_PLAYING;

		if (obj.segment) {
			sfx_song_set_status(&s->sound,
					    handle, setstate);
			PUT_SEL32V(obj, state, setstate);
		}
		break;
	}
	case _K_SCI01_SOUND_FADE_HANDLE :
	{
		/* There are four parameters that control the fade here.
		 * TODO: Figure out the exact semantics */
		break;
	}
	case _K_SCI01_SOUND_UPDATE_CUES :
	{
		/* FIXME: Fetch these from the sound server */

		int dataInc = 0;
		int signal = 1; /* This allows QfG2 to continue past the ogre */
		int min = 0;
		int sec = 0;
		int frame = 0;

		if (signal==0xff)
			/* Stop the handle */
		;

		if (dataInc!=GET_SEL32V(obj, dataInc))
		{
			PUT_SEL32V(obj, dataInc, dataInc);
			PUT_SEL32V(obj, signal, dataInc+0x7f);
		} else
		{
			PUT_SEL32V(obj, signal, signal);
		}

		PUT_SEL32V(obj, min, min);
		PUT_SEL32V(obj, sec, sec);
		PUT_SEL32V(obj, frame, frame);
		break;
	}
	case _K_SCI01_SOUND_PITCH_WHEEL :
	{
		break;
	}
	case _K_SCI01_SOUND_REVERB :
	{
		break;
	}
	case _K_SCI01_SOUND_CONTROLLER :
	{
		break;
	}
	}

	return s->r_acc;
}

reg_t
kDoSound_SCI1(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	word command = UKPV(0);

	switch (command)
	{
	case _K_SCI1_SOUND_MASTER_VOLME :
	{
	   /*int vol = UPARAM_OR_ALT (1, -1);

		if (vol != -1)
		        s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_VOLUME, 0, vol);
		else
		        s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_VOLUME, 0, 0);
			break;*/
	}
	case _K_SCI1_SOUND_MUTE_SOUND :
	{
		/* if there's a parameter, we're setting it.  Otherwise,
		   we're querying it. */
		/*int param = UPARAM_OR_ALT(1,-1);
		
		if (param != -1)
			s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_MUTE, 0, param);
		else
			s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_MUTE, 0, 0);
			break;*/
	}
	case _K_SCI1_SOUND_UNUSED1 :
	{
		break;
	}
	case _K_SCI1_SOUND_GET_POLYPHONY :
	{
		/*s->acc = s->sound_server->command(s, SOUND_COMMAND_TEST, 0, 0);*/
		break;
	}
	case _K_SCI1_SOUND_CD_AUDIO : 
	{
		break;
	}
	case _K_SCI1_SOUND_PLAY_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_INIT_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_DISPOSE_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_UPDATE_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_STOP_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_SUSPEND_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_FADE_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_HOLD_HANDLE :
	{
		break;
	}
	case _K_SCI1_SOUND_UNUSED2 :
	{
		break;
	}
	case _K_SCI1_SOUND_SET_HANDLE_VOLUME :
	{
		break;
	}
	case _K_SCI1_SOUND_SET_HANDLE_PRIORITY :
	{
		break;
	}
	case _K_SCI1_SOUND_SET_HANDLE_LOOP :
	{
		break;
	}
	case _K_SCI1_SOUND_UPDATE_CUES :
	{
		break;
	}
	case _K_SCI1_SOUND_MIDI_SEND :
	{
		break;
	}
	case _K_SCI1_SOUND_REVERB :
	{
		break;
	}
	case _K_SCI1_SOUND_UPDATE_VOL_PRI :
	{
		break;
	}
	}
	return s->r_acc;
}

reg_t
kDoSound(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	if (s->version>SCI_VERSION_FTU_DOSOUND_VARIANT_2)
		return kDoSound_SCI1(s, funct_nr, argc, argv);
	else if (s->version>SCI_VERSION_FTU_DOSOUND_VARIANT_1)
		return kDoSound_SCI01(s, funct_nr, argc, argv);
	else
		return kDoSound_SCI0(s, funct_nr, argc, argv);
}


