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

// As of January 2025, TypeScript lacks these definitions as well, unless they are added from an external package...
// I updated the definitions based on these two comments:
// - https://github.com/microsoft/TypeScript/issues/28308#issuecomment-1921865859
// - https://github.com/microsoft/TypeScript/issues/28308#issuecomment-1935417258

interface AudioWorkletProcessor {
	readonly port: MessagePort;
}

interface AudioWorkletProcessorImpl extends AudioWorkletProcessor {
	process(
		inputs: Float32Array[][],
		outputs: Float32Array[][],
		parameters: Record<string, Float32Array>
	): boolean;
}

type AudioParamDescriptor = {
	name: string,
	automationRate?: AutomationRate,
	minValue?: number,
	maxValue?: number,
	defaultValue?: number
};

declare var AudioWorkletProcessor: {
	prototype: AudioWorkletProcessor;
	new (options?: AudioWorkletNodeOptions): AudioWorkletProcessor;
};

interface AudioWorkletProcessorConstructor {
	new (options?: AudioWorkletNodeOptions): AudioWorkletProcessorImpl;
	parameterDescriptors?: AudioParamDescriptor[];
}

/** [MDN Reference](https://developer.mozilla.org/docs/Web/API/AudioWorkletGlobalScope/currentFrame) */
declare var currentFrame: number;

/** [MDN Reference](https://developer.mozilla.org/docs/Web/API/AudioWorkletGlobalScope/currentTime) */
declare var currentTime: number;

/** [MDN Reference](https://developer.mozilla.org/docs/Web/API/AudioWorkletGlobalScope/sampleRate) */
declare var sampleRate: number;

/** [MDN Reference](https://developer.mozilla.org/docs/Web/API/AudioWorkletGlobalScope/registerProcessor) */
declare function registerProcessor(
	name: string,
	processorCtor: AudioWorkletProcessorConstructor,
): void;
