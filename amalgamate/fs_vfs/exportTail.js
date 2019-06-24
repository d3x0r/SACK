
		function UTF8ArrayToString(u8Array, idx, maxBytesToRead) {
            maxBytesToRead = maxBytesToRead | 0;
            var endIdx = idx + maxBytesToRead;
            var endPtr = idx;
            // TextDecoder needs to know the byte length in advance, it doesn't stop on null terminator by itself.
            // Also, use the length info to avoid running tiny strings through TextDecoder, since .subarray() allocates garbage.
            // (As a tiny code save trick, compare endPtr against endIdx using a negation, so that undefined means Infinity)
            if( maxBytesToRead <= 0 )
                while (u8Array[endPtr]) ++endPtr;
            else
                endPtr = endIdx;
    
            if (endPtr - idx > 16 && u8Array.subarray && UTF8Decoder) {
                return UTF8Decoder.decode(u8Array.subarray(idx, endPtr));
            } else {
                var str = '';
                // If building with TextDecoder, we have already computed the string length above, so test loop end condition against that
                while (idx < endPtr) {
                // For UTF8 byte structure, see:
                // http://en.wikipedia.org/wiki/UTF-8#Description
                // https://www.ietf.org/rfc/rfc2279.txt
                // https://tools.ietf.org/html/rfc3629
                var u0 = u8Array[idx++];
                if (!(u0 & 0x80)) { str += String.fromCharCode(u0); continue; }
                var u1 = u8Array[idx++] & 63;
                if ((u0 & 0xE0) == 0xC0) { str += String.fromCharCode(((u0 & 31) << 6) | u1); continue; }
                var u2 = u8Array[idx++] & 63;
                if ((u0 & 0xF0) == 0xE0) {
                    u0 = ((u0 & 15) << 12) | (u1 << 6) | u2;
                } else {
                    if ((u0 & 0xF8) != 0xF0) warnOnce('Invalid UTF-8 leading byte 0x' + u0.toString(16) + ' encountered when deserializing a UTF-8 string on the asm.js/wasm heap to a JS string!');
                    u0 = ((u0 & 7) << 18) | (u1 << 12) | (u2 << 6) | (u8Array[idx++] & 63);
                }
    
                if (u0 < 0x10000) {
                    str += String.fromCharCode(u0);
                } else {
                    var ch = u0 - 0x10000;
                    str += String.fromCharCode(0xD800 | (ch >> 10), 0xDC00 | (ch & 0x3FF));
                }
                }
            }
            return str;
            }
    

const SACK = {};

var tempDouble = 0.0;
var tempI64 = 0;


module.exports = exports = Module;
//export {Module};
