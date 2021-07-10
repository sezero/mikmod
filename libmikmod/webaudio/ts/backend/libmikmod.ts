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

interface LibMikModCLib {
	HEAP8: Uint8Array;
	HEAPF32: Float32Array;

	_init(): number;
	_freeModule(): void;
	_terminate(): void;
	_preLoadModule(length: number): number;
	_loadModule(mixfreq: number, reverb: number, hqMixer: number, interpolation: number, noiseReduction: number, wrap: number, loop: number, fadeout: number): number;
	_changeGeneralOptions(reverb: number, interpolation: number, noiseReduction: number): void;
	_update(): number;
	_getSongName(): number;
	_getSongNameLength(): number;
	_getModType(): number;
	_getModTypeLength(): number;
	_getComment(): number;
	_getCommentLength(): number;
	_getAudioBuffer(): number;
	_getAudioBufferMaxLength(): number;
	_getAudioBufferUsedLength(): number;
}

class LibMikMod {
	private static cLib: LibMikModCLib | null = null;

	private static audioBufferPtr = 0;
	private static audioBufferUsedLength = 0;

	private static lastReverb = 0;
	private static lastHqMixer = 1;
	private static lastInterpolation = 1;
	private static lastNoiseReduction = 1;
	private static lastWrap = 0;
	private static lastLoop = 0;
	private static lastFadeout = 1;

	public static loading = false;
	public static loaded = false;
	public static loadError = false;

	public static init(wasmBinary: ArrayBuffer): Promise<void> {
		return (LibMikMod.loaded ? Promise.resolve() : new Promise((resolve, reject) => {
			if (LibMikMod.loading) {
				reject("Still loading");
				return;
			}

			if (LibMikMod.loadError) {
				reject("Error loading the library");
				return;
			}

			LibMikMod.loading = true;

			LibMikModCLib({ wasmBinary }).then((value) => {
				LibMikMod.cLib = value;

				const r = value._init();

				if (!r) {
					LibMikMod.loading = false;
					LibMikMod.loaded = true;
					resolve();
				} else {
					LibMikMod.loading = false;
					LibMikMod.loadError = true;
					reject(r);
				}
			}, (reason) => {
				LibMikMod.loading = false;
				LibMikMod.loadError = true;
				reject(reason);
			});
		}));
	}

	public static terminate(): void {
		if (LibMikMod.cLib) {
			LibMikMod.cLib._terminate();
			LibMikMod.cLib = null;

			LibMikMod.audioBufferPtr = 0;
			LibMikMod.audioBufferUsedLength = 0;
		}
	}

	public static loadModule(destinationSampleRate: number, srcBuffer?: ArrayBuffer | Uint8Array, options?: LibMikModLoadOptions | null): number {
		if (!LibMikMod.cLib)
			return 3; // MMERR_DYNAMIC_LINKING

		if (!srcBuffer)
			return 2; // MMERR_OUT_OF_MEMORY

		const ptr = LibMikMod.cLib._preLoadModule(srcBuffer.byteLength);
		if (!ptr)
			return 2; // MMERR_OUT_OF_MEMORY

		const dstBuffer = new Uint8Array(LibMikMod.cLib.HEAP8.buffer, ptr, srcBuffer.byteLength);

		if ("set" in srcBuffer)
			dstBuffer.set(srcBuffer, 0);
		else
			dstBuffer.set(new Uint8Array(srcBuffer, 0, srcBuffer.byteLength), 0);

		LibMikMod.audioBufferPtr = 0;
		LibMikMod.audioBufferUsedLength = 0;

		if (options) {
			if (options.reverb !== undefined && options.reverb >= 0 && options.reverb <= 15)
				LibMikMod.lastReverb = options.reverb;

			if (options.hqMixer !== undefined)
				LibMikMod.lastHqMixer = (options.hqMixer ? 1 : 0);

			if (options.interpolation !== undefined)
				LibMikMod.lastInterpolation = (options.interpolation ? 1 : 0);

			if (options.noiseReduction !== undefined)
				LibMikMod.lastNoiseReduction = (options.noiseReduction ? 1 : 0);

			if (options.wrap !== undefined)
				LibMikMod.lastWrap = (options.wrap ? 1 : 0);

			if (options.loop !== undefined)
				LibMikMod.lastLoop = (options.loop ? 1 : 0);

			if (options.fadeout !== undefined)
				LibMikMod.lastFadeout = (options.fadeout ? 1 : 0);
		}

		const r = LibMikMod.cLib._loadModule(destinationSampleRate, LibMikMod.lastReverb, LibMikMod.lastHqMixer, LibMikMod.lastInterpolation, LibMikMod.lastNoiseReduction, LibMikMod.lastWrap, LibMikMod.lastLoop, LibMikMod.lastFadeout);
		if (!r) {
			const audioBufferPtr = LibMikMod.cLib._getAudioBuffer();
			if (!audioBufferPtr)
				return 2; // MMERR_OUT_OF_MEMORY
		}

		return r;
	}

	public static changeGeneralOptions(options?: LibMikModGeneralOptions | null): void {
		if (options) {
			if (options.reverb !== undefined && options.reverb >= 0 && options.reverb <= 15)
				LibMikMod.lastReverb = options.reverb;

			if (options.interpolation !== undefined)
				LibMikMod.lastInterpolation = (options.interpolation ? 1 : 0);

			if (options.noiseReduction !== undefined)
				LibMikMod.lastNoiseReduction = (options.noiseReduction ? 1 : 0);

			if (LibMikMod.cLib)
				LibMikMod.cLib._changeGeneralOptions(LibMikMod.lastReverb, LibMikMod.lastInterpolation, LibMikMod.lastNoiseReduction);
		}
	}

	public static stopModule(): void {
		if (LibMikMod.cLib) {
			LibMikMod.cLib._freeModule();

			LibMikMod.audioBufferPtr = 0;
			LibMikMod.audioBufferUsedLength = 0;
		}
	}

	private static getString(ptr: number, len: number): string | null {
		if (LibMikMod.cLib && ptr) {
			if (!len)
				return "";

			const arr: number[] = new Array(len),
				heap = LibMikMod.cLib.HEAP8;

			while (len-- > 0)
				arr[len] = heap[ptr + len];

			return String.fromCharCode.apply(String, arr);
		}

		return null;
	}

	public static getSongName(): string | null {
		return (LibMikMod.cLib ? LibMikMod.getString(LibMikMod.cLib._getSongName(), LibMikMod.cLib._getSongNameLength()) : null);
	}

	public static getModType(): string | null {
		return (LibMikMod.cLib ? LibMikMod.getString(LibMikMod.cLib._getModType(), LibMikMod.cLib._getModTypeLength()) : null);
	}

	public static getComment(): string | null {
		return (LibMikMod.cLib ? LibMikMod.getString(LibMikMod.cLib._getComment(), LibMikMod.cLib._getCommentLength()) : null);
	}

	public static process(outputs: Float32Array[][]): boolean {
		if (!LibMikMod.cLib)
			return false;

		let audioBufferUsedLength = LibMikMod.audioBufferUsedLength;

		if (!audioBufferUsedLength) {
			for (let attempts = 0; attempts < 3; attempts++) {
				audioBufferUsedLength = LibMikMod.cLib._update();

				if (audioBufferUsedLength < 0)
					return false;

				if (audioBufferUsedLength) {
					LibMikMod.audioBufferPtr = LibMikMod.cLib._getAudioBuffer();
					LibMikMod.audioBufferUsedLength = audioBufferUsedLength;
					break;
				}
			}

			// Output silence if we cannot fill the buffer
			if (!audioBufferUsedLength)
				return true;
		}

		let tmpBuffer: Float32Array | null = null;

		for (let o = outputs.length - 1; o >= 0; o--) {
			const channels = outputs[o];

			for (let c = channels.length - 1; c >= 0; c--) {
				const channel = channels[c];

				if (!tmpBuffer) {
					// Convert mono 32-bit float samples into bytes
					const maxBytes = channel.length << 2;

					if (audioBufferUsedLength > maxBytes)
						audioBufferUsedLength = maxBytes;

					// Convert bytes into mono 32-bit float samples
					tmpBuffer = new Float32Array(LibMikMod.cLib.HEAP8.buffer, LibMikMod.audioBufferPtr, audioBufferUsedLength >> 2);
					LibMikMod.audioBufferPtr += audioBufferUsedLength;
					LibMikMod.audioBufferUsedLength -= audioBufferUsedLength;
				}

				channel.set(tmpBuffer);
			}
		}

		return true;
	}
}
