# XSWS

This was Xor, Sub, Wipe(LR,sub,RL), Sub.  While this had slightly higher performance, it produced
encrypted data with an obvious period.  This algorithm can be applied to buffer in-place.

# X(WS)*(WS)*

Revision 2
  - Moves byte map swap into wipe stage.

This is an encryption algorithm called Xor-Sub-Wipe-Sub (XSWS). 
 - Xor - the initial masking of data with a 256 bit mask
 - Wipe-subByte ( xor each byte with the previous byte L->R ). (use 0x55 as first prvious byte)
 - Wipe-subByte ( xor each byte with the previous byte R->L ). (use 0xAA as the first previous byte)

 
It is built on Salty Random Generator(SRG).  SRG uses various hash
algorithms to generate streams of random bits.  It then consumes bits
generated from the hashing function.  This specifically uses K12.
There really isn't any specific function SRG does that can't just
be done directly against the hash digests.

Byte 0 is least significant byte, and is consumed from hashes first.
bit 0 is the least value (1) of byte 0 is the very first bit consumed.
Otherwise all other operations are on parallel bytes, and endian-ness
should not matter; all final entropy reads are done in byte sized units.

## Overview

Take an input if less than 4096 bytes, add one byte to the length, and pad
to 8 byte boundary (int64); the length of the pad is stored in the last
byte; unused padding bytes will be set to 0.  (Block Padding...)[#Block_padding]

### Summary

   - Simple xor of a 256 bit mask computed from the key using KangarooTwelve(K12) over the key data.
   - Setup 256 byte swap map. (M)
   - xor 0x55 into first byte, swap byte for some other byte, store that result, use that result to xor on the next byte, repeatedly.
       1. T(0) = B(0) ^ 0x55
       2. R(0) = map T(0) byte to another byte using M
       3. T(N) = B(N) ^ R(N-1)
       4. R(N) = map T(N) byte to another byte using M
       5. repeat  3-4 until no byte bytes
   - xor 0xAA into last byte, swap byte for some other byte, store that result, use that result to xor on the previous byte, repeatedly.
       1. T(N) = B(N) ^ 0xAA
       2. R(N) = map T(N) byte to naother byte using M
       3. T(N-1) = B(N-1) ^ R(N)
       4. R(N-1) = map T(N-1) byte to naother byte using M
       5. repeat  3-4 until first byte
 
   - T is a temporary value
   - B is input Byte 
   - R is Result byte
   - N represents position in a string of bytes
   - M is the xbox map built below
 
Data is processed in blocks of 4096 bytes.  XOR pass uses wide integers to reduce time.  Blocks
MUST be padded to a multiple of 8 bytes.  Blocks that ARE a multiple of 8 bytes may or may not have
padding.  This allows multiple sections of large data to be processed in parallel.
A 1 bit change in the plain text will only affect the 4096 byte block it is in; but will on average
cause 16384 (of 32768) bits to change in the output.


## xbox generation

The XBox is a unique 2way exchange mapping of input A to output B, and B to A.  
This is done with 1 512 byte array.  This is the mapping used for 'swap byte with
another random byte'.  The first 256 bytes map A->B, and the second 256 bytes map B->A.
The second 256 bytes are derrived from the first 256 bytes.

This uses 256*8 bits of entropy and does a quick in-place swapping algorithm
to shuffle the bytes.  The first 256 bits of entropy are used for masking the
blocks initially, followed by these.

This is done with the same random_context as the source hash.


```
struct byte_shuffle_key {
    uint8_t map[256], dmap[256];
};

struct byte_shuffle_key *BlockShuffle_ByteShuffler( struct random_context *ctx ) {
	struct byte_shuffle_key *key = New( struct byte_shuffle_key );
	int n;
	int srcMap;

	// initialize map with default 1:1 map
	for( n = 0; n < 256; n++ )
		key->map[n] = n;

	// simple-in-place shuffler.
	for( n = 0; n < 256; n++ ) {
		int m; // the position to move
		int t; // temp
		SRG_GetByte_( m, ctx ); // m = next random 8 bits
		t = key->map[m];
		key->map[m] = key->map[n];
		key->map[n] = t;
	}

	/* generate reverse map */
	for( n = 0; n < 256; n++ )
		key->dmap[key->map[n]] = n;

	return key;
}
```

## Block padding...

Encryption is done in up to 4096 byte blocks.  

One(1) byte is added to the length, and then padded up to the next 8 byte boundary.

If the total block of data is larger than 4096 bytes; then each 4096 byte block is processed
separately.

The last byte of the last block is the number of bytes to remove from the end.  Other bytes 
added to pad to 8 byte boundary (will be 0 in this implementation, though there is no specific
requirement to be 0).


```
// 
encrypt( data, datalen ) { 
	int outlen = datalen + 1;
	
	if( outlen & 7 )
		outlen += ( 8 - (outlen&7) );
	
	char *output = malloc( outlen );
	((uin64_t*)( output + outlen-8 ))[0] = 0; // make sure all extra pad bytes are 0
	output[outlen-1] = outlen - datalen; // save pad to recover length.
	memcpy( output, data, datalen ); // encryption happens in-place.

	/* ... do real work ... */
}
```
 
## Algorithm Setup

The, current, strongest, fastest setup is done with K12.  Internally there is
support to change the CSPRNG; (SHA[1|2_256|2_512|3]|k12).

The algorithm performs quite a bit of setup before applying the transform
to data. There is a mask generated (basically) with 256 bits from K12(key data).

The same K12 hash generator is used from the initial seeding of the specified key information
is used for the mask and byte subtitution mapping creation.

```
	K12.Init()
	K12.Update( key );
	K12.Final();
	mask_entropy = K12.Squeeze( 256 );     // bits
	shuffle_entropy = K12.Squeeze( 256*8 );  // bits
```


__The other pre-generated data is the xbox, which uses 25+86 bits from the
same K12 bitstream to shuffle the substitution map (**)[xbox-generation]. __
 


## Detail of the algorithm

input keys of < hash size bits are the same as a buffer filled with 0's 
to pad to length of hash.  ( a 3-bit 0 is same as a 0-bit 0 ).

mask = 256 bit hash (K12) based on input key of 0-N bits.
xbox = Short for 'exchange box' to differentiate from s-box.  
       2 maps for byte A->B and B->A.  based on input key of 0-N bits.


```
^   = xor with above value
=   = temporary state observation point 

xboxSub = A->B operation
xboxBus = B->A operation

    a-h   = plain text
    Ac-Hc = plain text ^ mask; initical (c)ipher text
    A-H   = Ac-Hc substituted through xboxSub
    5     = 0x55    
    0     = 0xAA      //  (10 = 0)
    A'-H' = each byte xor next byte from first with first = 0x55
            (*' includes subsitution after running xor
             this was later added after expanding bare LR->RL XOR )
    A"-H" = each byte xor prev byte from last  with last = 0xAA
    Af-Hf = (f)inal cipher text output 
	 	
		
given     a b c d   e f g h 
         
       ^  CiphA-D   CiphE-H     apply mask to plaintext
	     
       =  AcBcCcDc  EcFcGcHc    masked text
	     
 xboxSub     - xbox -           masked text swapped through xbox
         

// The following is a representation of the composite of each value.
// a (proof) that every byte affects every other byte withiin
// a message.

       =  A B C D   E F G H     a^k, b^k,...
       ^  5 A B C   D E F G
       ^    5 A B   C D E F
       ^      5 A   B C D E
       ^        5   A B C D
       ^            5 A B C
       ^              5 A B
       ^                5 A
       ^                  5 

 xboxSub     - xbox -           xor'd sum then each byte swapped through xbox
         
       =  A'B'C'D'  E'F'G'H'    above columns xor'd together (AND SWAPPED)


	   
       ^  B'C'D'E   F'G'H'0
       ^  C'D'E'F   G'H'0 
       ^  D'E'F'G   H'0   
       ^  E'F'G'H   0      
	 	  	 					   
       ^  F'G'H'0         
       ^  G'H'0           
       ^  H'0             
       ^  0   
         
          A"B"C"D"  E"F"G"H"    above columns xor'd together
		 
 xboxSub     - xbox -           wiped text swap through xbox
                                (this could also be xboxBus instead)
         
          AfBfCfDf  EfFfGfHf    final result after one final swap matrix.
	   
```
	   
```	
	/*
	   output buffer MUST be padded to 64 bits (8 bytes).
	   output buffer must be at least 1 byte larger than input buffer.
	   the last output buffer byte is the pad length.
	*/
	// masking can be done in wide blocks
	for( n = 0; n < outlen; n += 8, output += 8 )
		((uint64_t*)output)[0] ^= ((uint64_t*)(bufKey + (n % 32)))[0];
	   
	BlockShuffle_SubBytes_( bytKey, output, output, outlen );

	for( n = 0, p = 0x55; n < outlen; n++, output++ ) 
		p = output[0] = ( output[0] ^ p );

	BlockShuffle_SubBytes_( bytKey, output, output, outlen );

	output--; // back up 1 byte.
	for( n = 0, p = 0xAA; n < outlen; n++, output-- )  p = output[0] = output[0] ^ p;
	   
	BlockShuffle_SubBytes_( bytKey, output, output, outlen );
```
	

Decode is the inverse of the ALL above steps, using BlockShuffle_BusBytes (Reverse of Sub)
for the swap instead. (xor-wipe is inversed too)


```
	BlockShuffle_BusBytes_( bytKey, input, output, len );

	uint8_t *curBuf = output;
	for( n = 0; n < (len - 1); n++, curBuf++ ) {
		curBuf[0] = curBuf[0] ^ curBuf[1];
	}
	curBuf[0] = curBuf[0] ^ 0xAA;
	
	BlockShuffle_BusBytes_( bytKey, output, output, len );

	curBuf = output + len - 1;
	for( n = (len - 1); n > 0; n--, curBuf-- ) {
		curBuf[0] = curBuf[0] ^ curBuf[-1];
	}
	curBuf[0] = curBuf[0] ^ 0x55;

	BlockShuffle_BusBytes_( bytKey, output, output, len );

	// masking can be done in wide blocks
	for( n = 0; n < len; n += 8, output += 8 ) {
		((uint64_t*)output)[0] ^= ((uint64_t*)(bufKey + (n % (RNGHASH / 8))))[0];;
	}
	   
```

---

# Historical and development notes

Disregard from here down.

---

byte permuations

2^256 ...(ish)
116,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000.

256! (factorial)
2^1684 (?) bits

8578177753428426541190822716812326251577815202794856198596556503772694525531475893774402913604514084
5037588534233658430615719683469369647532228928849742602567963733256336878644267520762679456018796886
7971521143307702077526646451464709187326100832876325702818980773671781454170250523018608495319068138
2574810702528175594594769870346657127381392862052347568082188607012036110831520935019474371091017269
68262861606263662435022840944191408424615936000000000000000000000000000000000000000000000000000000000000000

9a 4b 7d ce 03 6e 09 9d 61 7f 2b 0e 61 2f a7 e4 0e 2a f5 b9 1b 58 bf 3d 92 0e df e6 79 c9 85 1d  šK}Î.n..a.+.a/§ä.*õ..X¿=’.ßæyÉ..
6a 50 21 b6 db 56 ea 5c 9a bd e6 45 

128 * 8+64*7+32*6+16*5+8*4+4*3+2*2+1*1
1793
2048

64/4 

64*6
24 * 4

---

For tiny blocks (44 bytes), AES is quite a bit faster (down to only like 7 times now)
For larger blocks, MyShuffle is faster.

1) the routine to remap the bytes is actually VERY slow. (it's not doing a lot of work, just... is slow)
2) I was shuffling the 256 card deck, assigning a random 8 bit number (0-255) and then sorting them based 
that number... that uses 2048 bits of randomness (calculates 4 K12 digests)... I pondered this for a while,
and reworked it so it uses only 148 bits to shuffle the cards.

2a ) split the 'deck' into left and right piles (top/bottom)

2b ) split each of those piles into 3 sections.  (each 1/3 of each half is mixed with a paired 1/3 of the other half)

2b1 ) on the left/top half, random order the piles so that 0 is not the first one (1,2,0)(1,0,2)(2,1,0)(2,0,1)  (keeps the top card from staying the top card)
2b2 ) on the right/bottom half, random order the piles so that 2 is not the last one (2,0,1)(2,1,0)(1,2,0)(0,2,1)  
2b3 ) choosing 1 of the 4 orders is only 2 bits (0,1,2,3); (twice, one order for left, one for right) 4 bits per round of shuffling.

2c ) flip a coin (8) times, if it's heads, use the left pile first, else use right pile first.

2d ) for each card in the 1/3 stack of left/right split... flip a coin.  Every time the coin is 'heads', switch which pile to pull cards from, saving the total cards pulled on the previous pile.
2c1 ) (label this countStack) (assume left is counted first)  
      T ( count:1 don't change  )
	  H ( count:2  store 2(from Left), reset count, change stacks : RIGHT )
	  H ( count:1  store 1(from Right), reset count, change stacks : LEFT )
      T ( count:1 don't change  )
      T ( count:2 don't change  )
	  H ( count:3  store 3(from Left), reset count, change stacks : RIGHT )
      T ( count:1 don't change  )
	  H ( count:2  store 2(from Right), reset count, change stacks : LEFT )
	  .... etc.
	  
3) for N rounds of shuffles.... for each card to shuffle

3a)   if( left ) use left stack, add one to count util the count in countStack is reached. switch to right.
3b)   if( right ) use right stack, add one to count util the count in countStack is reached.

3c)   if [left/right] 1/3 cut stack runs out of cards, then use the rest of the cards in the 1/3 cut stack, move to next third.

3d )  switch in-deck and out-deck, repeat round shuffle on this into next out-deck.


theoretial bit count : ( 8 * 4 ) + 8 + 86 + (86 * 0.001 /*add .1% */)  == (40 + 86.08) == [ 127 ]  

(using 127 bits is much better than 2048 :) )

And, man, does it take a lot of words to explain 'riffle shuffle' :)




My Shuffle/[En/De]crypt

DID 100000 in 6694ms   14938/Sec  16384 bytes
DID 100000 in 1605ms   62305/Sec 2048 bytes
DID 100000 in 837ms   119474/Sec 43 bytes
DID 100000 in 830ms   120481/Sec  43 bytes


AES [En/De]crypt

DID 100000 in 15689ms   6373/Sec  16384 bytes
DID 100000 in 2065ms   48426/Sec  2048 bytes
DID 100000 in 143ms   699300/Sec  43 bytes
DID 100000 in 129ms   775193/Sec  43 bytes


TESTDATA
48 65 6c 6c 6f 2c 20 54 68 69 73 20 69 73 20 61    Hello, This is a
20 74 65 73 74 2c 20 74 68 69 73 20 69 73 20 4f     test, this is O
6e 6c 79 20 61 20 74 65 73 74 2e 00                nly a test..
BINARY
d2 9d 2a ed a4 9b 5c 8e 82 1d 06 c6 47 e2 15 fa    ..*...\.....G...
77 f7 45 43 f5 dc 1d 74 1c 6f 40 00 e9 f7 c2 f8    w.EC...t.o@.....
21 30 77 96 d1 c6 56 5a 23 66 be b5                !0w...VZ#f..
ORIG
48 65 6c 6c 6f 2c 20 54 68 69 73 20 69 73 20 61    Hello, This is a
20 74 65 73 74 2c 20 74 68 69 73 20 69 73 20 4f     test, this is O
6e 6c 79 20 61 20 74 65 73 74 2e 00                nly a test..
BINARY - 1 bit change input
df 3e d9 18 89 69 b6 53 0e 17 5f ad 33 9c cf bb    .>...i.S.._.3...
39 3c 07 61 2e 86 17 1b 67 65 a8 de 56 3c 03 26    9<.a....ge..V<.&
23 94 bb 23 00 32 34 10 4b 5d 32 60                #..#.24.K]2`

                             pkt        bytes
         (packets)   MS    per-sec     per-sec     
(XSWS)
Tiny DID  300000 in 1257    238663    10,501,172
Tiny DID  900000 in 3437    261856    11,521,664
Big DID   300000 in 4171     71925   147,302,400
Big DID   300000 in 3239     92621   189,687,808
Mega DID     300 in 4956        60   251,658,240
Mega DID     300 in 6490        46   192,937,984
(Current)
Tiny DID  900000 in 2324    387263    17,039,572
Big DID   300000 in 2756    108853   222,930,944
Mega DID     300 in 3493        85   356,515,840

(2019-02-23 post sub change)
(x86)
Tiny DID  900000 in 3750   240000  10560000
Big DID   300000 in 3748    80042 163926016
Mega DID     300 in 5696       52 218103808
(x64)
Tiny DID  900000 in 1691   532229  23,418,076
Big DID   300000 in 2941   102006 208,908,288
Mega DID     300 in 5014       59 247,463,936


(AES)
tiny DID 4000000 in 3918   1020929    44,920,876
tiny DID 2000000 in 1956   1022494    44,989,736
Big DID   200000 in 4033     49590   101,560,320
Big DID   200000 in 4027     49664   101,711,872
Mega DID     100 in 4173        23    96,468,992
Mega DID     100 in 4151        24   100,663,296




/* 13 shuffle rounds instead of 5 */
Tiny DID 900000 in 7844   114737 5048428
Big DID 300000 in 4815   62305 127600640
Mega DID 300 in 6594   45 188743680

