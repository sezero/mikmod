/*	MikMod Web Audio library
	(c) 2021 Carlos Rafael Gimenes das Neves.

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

class LibMikModAudioNode {
	// Due to both LibMikMod and AudioWorklet's nature we can
	// have only one module loaded at a time...

	public static initialized = false;
	public static loading = false;
	public static loaded = false;
	public static loadError = false;

	public static infoSongName: string | null = null;
	public static infoModType: string | null = null;
	public static infoComment: string | null = null;

	private static currentId = 0;
	private static audioNode: AudioWorkletNode | null = null;

	private static wasmBuffer: ArrayBuffer | null = null;
	
	private static srcBuffer: ArrayBuffer | Uint8Array | null = null;
	private static loadOptions?: LibMikModLoadOptions | null = null;
	private static onload: ((audioNode: AudioNode) => void) | null = null;
	private static onerror: ((reason?: any) => void) | null = null;
	private static onended: (() => void) | null = null;

	public static isSupported(): boolean {
		// Should we also check for HTTPS? Because, apparently, the browser already undefines
		// AudioWorklet when not serving the files from a secure origin...
		return (("AudioWorklet" in window) && ("AudioWorkletNode" in window) && ("WebAssembly" in window));
	}

	public static async init(audioContext: AudioContext, libPath?: string | null): Promise<void> {
		if (LibMikModAudioNode.initialized)
			return;

		if (!libPath)
			libPath = "";
		else if (!libPath.endsWith("/"))
			libPath += "/";

		LibMikModAudioNode.initialized = true;

		const response = await fetch(libPath + "libmikmodclib.wasm");

		LibMikModAudioNode.wasmBuffer = await response.arrayBuffer();

		return audioContext.audioWorklet.addModule(libPath + "libmikmodprocessor.min.js");
	}

	private static postMessage(message: LibMikModMessage): void {
		if (!LibMikModAudioNode.audioNode)
			return;

		if (message.buffer)
			LibMikModAudioNode.audioNode.port.postMessage(message, [message.buffer]);
		else
			LibMikModAudioNode.audioNode.port.postMessage(message);
	}

	public static loadModule(audioContext: AudioContext, srcBuffer: ArrayBuffer | Uint8Array, onload: (audioNode: AudioNode) => void, onerror: (reason?: any) => void, onended: () => void, options?: LibMikModLoadOptions): void {
		if (LibMikModAudioNode.loading)
			throw new Error("Still loading");

		if (LibMikModAudioNode.loadError)
			throw new Error("Error loading the library");

		if (!srcBuffer)
			throw new Error("Null buffer");

		const audioNode = new AudioWorkletNode(audioContext, "libmikmodprocessor");

		audioNode.port.onmessage = LibMikModAudioNode.handleResponse;
		audioNode.onprocessorerror = LibMikModAudioNode.notifyError;

		LibMikModAudioNode.audioNode = audioNode;

		LibMikModAudioNode.infoSongName = null;
		LibMikModAudioNode.infoModType = null;
		LibMikModAudioNode.infoComment = null;

		LibMikModAudioNode.srcBuffer = srcBuffer;
		LibMikModAudioNode.onload = onload;
		LibMikModAudioNode.onerror = onerror;
		LibMikModAudioNode.onended = onended;
		LibMikModAudioNode.loadOptions = options;

		LibMikModAudioNode.loading = true;

		LibMikModAudioNode.postMessage({
			id: LibMikModAudioNode.currentId,
			messageId: LibMikModMessageId.GET_ID
		});
	}

	private static finishLoadingModule(): void {
		if (!LibMikModAudioNode.loaded) {
			if (!LibMikModAudioNode.wasmBuffer) {
				this.notifyError("Null wasmBuffer");
				return;
			}

			LibMikModAudioNode.postMessage({
				id: LibMikModAudioNode.currentId,
				messageId: LibMikModMessageId.INIT,
				buffer: LibMikModAudioNode.wasmBuffer
			});

			LibMikModAudioNode.wasmBuffer = null;
		} else {
			if (!LibMikModAudioNode.srcBuffer) {
				this.notifyError("Null srcBuffer");
				return;
			}

			LibMikModAudioNode.postMessage({
				id: LibMikModAudioNode.currentId,
				messageId: LibMikModMessageId.LOAD_MODULE_BUFFER,
				buffer: LibMikModAudioNode.srcBuffer,
				options: LibMikModAudioNode.loadOptions
			});

			LibMikModAudioNode.srcBuffer = null;
			LibMikModAudioNode.loadOptions = null;
		}
	}

	public static changeGeneralOptions(options?: LibMikModGeneralOptions): void {
		LibMikModAudioNode.postMessage({
			id: LibMikModAudioNode.currentId,
			messageId: LibMikModMessageId.CHANGE_GENERAL_OPTIONS,
			options: options
		});
	}

	public static stopModule(): void {
		if (!LibMikModAudioNode.audioNode)
			return;

		LibMikModAudioNode.postMessage({
			id: LibMikModAudioNode.currentId,
			messageId: LibMikModMessageId.STOP_MODULE
		});

		LibMikModAudioNode.currentId = 0;

		LibMikModAudioNode.audioNode = null;
		LibMikModAudioNode.srcBuffer = null;

		LibMikModAudioNode.onload = null;
		LibMikModAudioNode.onerror = null;
		LibMikModAudioNode.onended = null;
	}

	private static notifyError(error?: any): void {
		if (!LibMikModAudioNode.audioNode)
			return;

		LibMikModAudioNode.currentId = 0;

		LibMikModAudioNode.audioNode = null;
		LibMikModAudioNode.srcBuffer = null;

		const onerror = LibMikModAudioNode.onerror;

		LibMikModAudioNode.onload = null;
		LibMikModAudioNode.onerror = null;
		LibMikModAudioNode.onended = null;

		if (onerror)
			onerror(error);
	}

	private static notifyEnded(): void {
		if (!LibMikModAudioNode.audioNode)
			return;

		LibMikModAudioNode.currentId = 0;

		LibMikModAudioNode.audioNode = null;
		LibMikModAudioNode.srcBuffer = null;

		const onended = LibMikModAudioNode.onended;

		LibMikModAudioNode.onload = null;
		LibMikModAudioNode.onerror = null;
		LibMikModAudioNode.onended = null;

		if (onended)
			onended();
	}

	private static handleResponse(ev: MessageEvent): void {
		const message = ev.data as LibMikModResponse;

		if (!message)
			return;

		switch (message.messageId) {
			case LibMikModMessageId.INIT:
				if (LibMikModAudioNode.loading)
					LibMikModAudioNode.postMessage({
						id: LibMikModAudioNode.currentId,
						messageId: LibMikModMessageId.CHECK_STATUS
					});
				break;

			case LibMikModMessageId.CHECK_STATUS:
				if (message.id !== LibMikModAudioNode.currentId)
					break;

				if (message.loading) {
					setTimeout(() => {
						LibMikModAudioNode.postMessage({
							id: LibMikModAudioNode.currentId,
							messageId: LibMikModMessageId.CHECK_STATUS
						});
					}, 10);
				} else if (message.loaded) {
					LibMikModAudioNode.loaded = true;

					if (LibMikModAudioNode.srcBuffer) {
						LibMikModAudioNode.postMessage({
							id: LibMikModAudioNode.currentId,
							messageId: LibMikModMessageId.LOAD_MODULE_BUFFER,
							buffer: LibMikModAudioNode.srcBuffer,
							options: LibMikModAudioNode.loadOptions
						});
			
						LibMikModAudioNode.srcBuffer = null;
						LibMikModAudioNode.loadOptions = null;
					} else {
						LibMikModAudioNode.loading = false;
					}
				} else {
					LibMikModAudioNode.loading = false;
					LibMikModAudioNode.loadError = true;

					LibMikModAudioNode.notifyError();
				}
				break;

			case LibMikModMessageId.LOAD_MODULE_BUFFER:
				if (message.id !== LibMikModAudioNode.currentId)
					break;

				LibMikModAudioNode.loading = false;

				if (LibMikModAudioNode.onload && LibMikModAudioNode.onerror && LibMikModAudioNode.audioNode) {
					if (message.result) {
						LibMikModAudioNode.notifyError(message.result);
					} else {
						LibMikModAudioNode.infoSongName = (message.infoSongName || null);
						LibMikModAudioNode.infoModType = (message.infoModType || null);
						LibMikModAudioNode.infoComment = (message.infoComment || null);

						LibMikModAudioNode.onload(LibMikModAudioNode.audioNode);
					}
				}
				break;

			case LibMikModMessageId.PLAYBACK_ENDED:
				if (message.id !== LibMikModAudioNode.currentId)
					break;

				LibMikModAudioNode.notifyEnded();
				break;

			case LibMikModMessageId.GET_ID:
				LibMikModAudioNode.currentId = message.id;
				LibMikModAudioNode.finishLoadingModule();
				break;
		}
	}
}
