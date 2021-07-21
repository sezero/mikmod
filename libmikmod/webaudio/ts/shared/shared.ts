/*	MikMod Web Audio library
	(c) 2021 Carlos Rafael Gimenes das Neves.

	https://github.com/sezero/mikmod
	https://github.com/carlosrafaelgn/mikmod/tree/master/libmikmod/webaudio

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

interface LibMikModGeneralOptions {
	reverb?: number;
	interpolation?: boolean;
	noiseReduction?: boolean;
}

interface LibMikModLoadOptions extends LibMikModGeneralOptions {
	hqMixer?: boolean;
	wrap?: boolean;
	loop?: boolean;
	fadeout?: boolean;
}

enum LibMikModMessageId {
	INIT = 1,
	CHECK_STATUS = 2,
	LOAD_MODULE_BUFFER = 3,
	CHANGE_GENERAL_OPTIONS = 4,
	STOP_MODULE = 5,
	PLAYBACK_ERROR = 6,
	PLAYBACK_ENDED = 7,
	GET_ID = 8
}

interface LibMikModMessage {
	id: number;
	messageId: LibMikModMessageId;
	options?: LibMikModGeneralOptions | LibMikModLoadOptions | null;
	buffer?: ArrayBuffer;
}

interface LibMikModResponse {
	id: number;
	messageId: LibMikModMessageId;
	errorCode?: number;
	errorStr?: string | null;
	infoSongName?: string | null;
	infoModType?: string | null;
	infoComment?: string | null;
	loading?: boolean;
	loaded?: boolean;
	loadError?: boolean;
}
