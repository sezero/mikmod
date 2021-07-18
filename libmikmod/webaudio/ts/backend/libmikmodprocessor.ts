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

// https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletGlobalScope
// https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletProcessor/process
// https://developers.google.com/web/updates/2017/12/audio-worklet
// https://developers.google.com/web/updates/2018/06/audio-worklet-design-pattern
// https://github.com/GoogleChromeLabs/web-audio-samples/tree/main/audio-worklet

class LibMikModProcessor extends AudioWorkletProcessor {
	// Due to both LibMikMod and AudioWorklet's nature we can
	// have only one module loaded at a time...

	private static currentId = 0;

	private readonly id: number;
	private ended = false;

	public constructor() {
		super();

		this.id = ++LibMikModProcessor.currentId;
		this.port.onmessage = this.handleMessage.bind(this);
	}

	private postResponse(response: LibMikModResponse): void {
		if (this.id == LibMikModProcessor.currentId)
			this.port.postMessage(response);
	}

	private handleMessage(ev: MessageEvent): any {
		const message = ev.data as LibMikModMessage;

		if (!message)
			return;

		switch (message.messageId) {
			case LibMikModMessageId.INIT:
				if (!LibMikMod.loaded && !LibMikMod.loading && !LibMikMod.loadError && message.buffer) {
					LibMikMod.init(message.buffer).then(() => {
						this.postResponse({
							id: this.id,
							messageId: LibMikModMessageId.INIT
						});
					}, () => {
						this.postResponse({
							id: this.id,
							messageId: LibMikModMessageId.INIT
						});
					});
				}
				break;

			case LibMikModMessageId.CHECK_STATUS:
				if (message.id === this.id)
					this.postResponse({
						id: this.id,
						messageId: LibMikModMessageId.CHECK_STATUS,
						loading: LibMikMod.loading,
						loaded: LibMikMod.loaded,
						loadError: LibMikMod.loadError
					});
				break;

			case LibMikModMessageId.LOAD_MODULE_BUFFER:
				if (message.id === this.id) {
					const result = LibMikMod.loadModule(sampleRate, message.buffer, message.options);
					if (result) {
						this.postResponse({
							id: this.id,
							messageId: LibMikModMessageId.LOAD_MODULE_BUFFER,
							result
						});
					} else {
						this.postResponse({
							id: this.id,
							messageId: LibMikModMessageId.LOAD_MODULE_BUFFER,
							result,
							infoSongName: LibMikMod.getSongName(),
							infoModType: LibMikMod.getModType(),
							infoComment: LibMikMod.getComment()
						});
					}
				}
				break;

			case LibMikModMessageId.CHANGE_GENERAL_OPTIONS:
				LibMikMod.changeGeneralOptions(message.options);
				break;

			case LibMikModMessageId.STOP_MODULE:
				if (message.id === this.id && !this.ended) {
					this.ended = true;
					LibMikMod.stopModule();
				}
				break;

			case LibMikModMessageId.GET_ID:
				this.postResponse({
					id: this.id,
					messageId: LibMikModMessageId.GET_ID
				});
				break;
		}
	}

	public process(inputs: Float32Array[][], outputs: Float32Array[][], parameters: { [key: string]: Float32Array }): boolean {
		if (this.id != LibMikModProcessor.currentId || this.ended)
			return false;
		if (!LibMikMod.process(outputs)) {
			if (!this.ended) {
				this.ended = true;
				LibMikMod.stopModule();
				this.postResponse({
					id: this.id,
					messageId: (LibMikMod.getErrno() ? LibMikModMessageId.PLAYBACK_ERROR : LibMikModMessageId.PLAYBACK_ENDED),
					result: LibMikMod.getErrno()
				});
			}
			return false;
		}
		return true;
	}
}

registerProcessor("libmikmodprocessor", LibMikModProcessor);
