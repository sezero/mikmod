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

class LibMikMod {
	// Due to both LibMikMod and AudioWorklet's nature we can
	// have only one module loaded at a time...

	public static readonly ERROR_INITIALIZE = 1;
	public static readonly ERROR_FILE_READ = 2;
	public static readonly ERROR_MODULE_LOAD = 3;
	public static readonly ERROR_PLAYBACK = 4;

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
	private static onerror: ((errorCode: number, reason?: any) => void) | null = null;
	private static onended: (() => void) | null = null;

	public static isSupported(): boolean {
		// Should we also check for HTTPS? Because, apparently, the browser already undefines
		// AudioWorklet when not serving the files from a secure origin...
		return (("AudioWorklet" in window) && ("AudioWorkletNode" in window) && ("WebAssembly" in window));
	}

	public static async init(audioContext: AudioContext, libPath?: string | null): Promise<void> {
		if (LibMikMod.initialized)
			return;

		if (!libPath)
			libPath = "";
		else if (!libPath.endsWith("/"))
			libPath += "/";

		LibMikMod.initialized = true;

		const response = await fetch(libPath + "libmikmodclib.wasm");

		LibMikMod.wasmBuffer = await response.arrayBuffer();

		return audioContext.audioWorklet.addModule(libPath + "libmikmodprocessor.min.js");
	}

	private static postMessage(message: LibMikModMessage): void {
		if (!LibMikMod.audioNode)
			return;

		if (message.buffer)
			LibMikMod.audioNode.port.postMessage(message, [message.buffer]);
		else
			LibMikMod.audioNode.port.postMessage(message);
	}

	public static loadModule(audioContext: AudioContext, source: File | ArrayBuffer | Uint8Array, onload: (audioNode: AudioNode) => void, onerror: (errorCode: number, reason?: any) => void, onended: () => void, options?: LibMikModLoadOptions): void {
		if (LibMikMod.loading)
			throw new Error("Still loading");

		if (LibMikMod.loadError)
			throw new Error("Error loading the library");

		if (!source)
			throw new Error("Null source");

		const audioNode = new AudioWorkletNode(audioContext, "libmikmodprocessor");

		audioNode.port.onmessage = LibMikMod.handleResponse;
		audioNode.onprocessorerror = LibMikMod.notifyProcessorError;

		LibMikMod.audioNode = audioNode;

		LibMikMod.infoSongName = null;
		LibMikMod.infoModType = null;
		LibMikMod.infoComment = null;

		LibMikMod.onload = onload;
		LibMikMod.onerror = onerror;
		LibMikMod.onended = onended;
		LibMikMod.loadOptions = options;

		LibMikMod.loading = true;

		if ("lastModified" in source) {
			if ("arrayBuffer" in source) {
				source.arrayBuffer().then(LibMikMod.startLoadingModule, LibMikMod.notifyReaderError);
			} else {
				const reader = new FileReader();
				reader.onerror = LibMikMod.notifyReaderError;
				reader.onload = function () {
					if (!reader.result)
						LibMikMod.notifyReaderError();
					else
						LibMikMod.startLoadingModule(reader.result as ArrayBuffer);
				};
				reader.readAsArrayBuffer(source);
			}
		} else {
			LibMikMod.startLoadingModule(source);
		}
	}

	private static startLoadingModule(srcBuffer: ArrayBuffer | Uint8Array): void {
		LibMikMod.srcBuffer = srcBuffer;

		LibMikMod.postMessage({
			id: LibMikMod.currentId,
			messageId: LibMikModMessageId.GET_ID
		});
	}

	private static finishLoadingModule(): void {
		if (!LibMikMod.loaded) {
			if (!LibMikMod.wasmBuffer) {
				LibMikMod.notifyError(LibMikMod.ERROR_INITIALIZE, "Null wasmBuffer");
				return;
			}

			LibMikMod.postMessage({
				id: LibMikMod.currentId,
				messageId: LibMikModMessageId.INIT,
				buffer: LibMikMod.wasmBuffer
			});

			LibMikMod.wasmBuffer = null;
		} else {
			if (!LibMikMod.srcBuffer) {
				LibMikMod.notifyError(LibMikMod.ERROR_MODULE_LOAD, "Null srcBuffer");
				return;
			}

			LibMikMod.postMessage({
				id: LibMikMod.currentId,
				messageId: LibMikModMessageId.LOAD_MODULE_BUFFER,
				buffer: LibMikMod.srcBuffer,
				options: LibMikMod.loadOptions
			});

			LibMikMod.srcBuffer = null;
			LibMikMod.loadOptions = null;
		}
	}

	public static changeGeneralOptions(options?: LibMikModGeneralOptions): void {
		LibMikMod.postMessage({
			id: LibMikMod.currentId,
			messageId: LibMikModMessageId.CHANGE_GENERAL_OPTIONS,
			options: options
		});
	}

	public static stopModule(): void {
		if (!LibMikMod.audioNode)
			return;

		LibMikMod.postMessage({
			id: LibMikMod.currentId,
			messageId: LibMikModMessageId.STOP_MODULE
		});

		LibMikMod.currentId = 0;

		LibMikMod.audioNode = null;
		LibMikMod.srcBuffer = null;

		LibMikMod.onload = null;
		LibMikMod.onerror = null;
		LibMikMod.onended = null;
	}

	private static notifyReaderError(reason?: any): void {
		LibMikMod.notifyError(LibMikMod.ERROR_FILE_READ, reason);
	}

	private static notifyProcessorError(reason?: any): void {
		LibMikMod.notifyError(LibMikMod.ERROR_PLAYBACK, reason);
	}

	private static notifyError(errorCode: number, reason?: any): void {
		if (!LibMikMod.audioNode)
			return;

		LibMikMod.loading = false;

		LibMikMod.currentId = 0;

		LibMikMod.audioNode = null;
		LibMikMod.srcBuffer = null;

		const onerror = LibMikMod.onerror;

		LibMikMod.onload = null;
		LibMikMod.onerror = null;
		LibMikMod.onended = null;

		if (onerror)
			onerror(errorCode, reason);
	}

	private static notifyEnded(): void {
		if (!LibMikMod.audioNode)
			return;

		LibMikMod.currentId = 0;

		LibMikMod.audioNode = null;
		LibMikMod.srcBuffer = null;

		const onended = LibMikMod.onended;

		LibMikMod.onload = null;
		LibMikMod.onerror = null;
		LibMikMod.onended = null;

		if (onended)
			onended();
	}

	private static handleResponse(ev: MessageEvent): void {
		const message = ev.data as LibMikModResponse;

		if (!message)
			return;

		switch (message.messageId) {
			case LibMikModMessageId.INIT:
				if (LibMikMod.loading)
					LibMikMod.postMessage({
						id: LibMikMod.currentId,
						messageId: LibMikModMessageId.CHECK_STATUS
					});
				break;

			case LibMikModMessageId.CHECK_STATUS:
				if (message.id !== LibMikMod.currentId)
					break;

				if (message.loading) {
					setTimeout(function () {
						LibMikMod.postMessage({
							id: LibMikMod.currentId,
							messageId: LibMikModMessageId.CHECK_STATUS
						});
					}, 10);
				} else if (message.loaded) {
					LibMikMod.loaded = true;

					if (LibMikMod.srcBuffer) {
						LibMikMod.postMessage({
							id: LibMikMod.currentId,
							messageId: LibMikModMessageId.LOAD_MODULE_BUFFER,
							buffer: LibMikMod.srcBuffer,
							options: LibMikMod.loadOptions
						});
			
						LibMikMod.srcBuffer = null;
						LibMikMod.loadOptions = null;
					} else {
						LibMikMod.loading = false;
					}
				} else {
					LibMikMod.loading = false;
					LibMikMod.loadError = true;

					LibMikMod.notifyError(LibMikMod.ERROR_INITIALIZE);
				}
				break;

			case LibMikModMessageId.LOAD_MODULE_BUFFER:
				if (message.id !== LibMikMod.currentId)
					break;

				LibMikMod.loading = false;

				if (LibMikMod.onload && LibMikMod.onerror && LibMikMod.audioNode) {
					if (message.result) {
						LibMikMod.notifyError(LibMikMod.ERROR_MODULE_LOAD, message.result);
					} else {
						LibMikMod.infoSongName = (message.infoSongName || null);
						LibMikMod.infoModType = (message.infoModType || null);
						LibMikMod.infoComment = (message.infoComment || null);

						LibMikMod.onload(LibMikMod.audioNode);
					}
				}
				break;

			case LibMikModMessageId.PLAYBACK_ERROR:
				if (message.id !== LibMikMod.currentId)
					break;

				LibMikMod.notifyProcessorError(message.result);
				break;

			case LibMikModMessageId.PLAYBACK_ENDED:
				if (message.id !== LibMikMod.currentId)
					break;

				LibMikMod.notifyEnded();
				break;

			case LibMikModMessageId.GET_ID:
				LibMikMod.currentId = message.id;
				LibMikMod.finishLoadingModule();
				break;
		}
	}
}
