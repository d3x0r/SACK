# XSWS

This is an encryption algorithm called Xor-Sub-Wipe-Sub (XSWS). 
 - Xor - the initial masking of data with a mask
 - Sub - substituted masked bytes for some other random byte
 - Wipe - 
     - xor each byte with the previous byte L->R
	 - Sub each byte with some other random byte.
     - xor each byte with the previous byte R->L
 - Sub - substitute each byte with other random byte.

 
It is built on Salty Random Generator(SRG).  SRG uses various hash
algorithms to generate streams of random bits.  It then consumes bits
generated from the hashing function.  This specifically uses K12.
There really isn't any specific function SRG does that can't just
be done directly against the hash digests.

Byte 0 is least significant byte, and is consumed from hashes first.
bit 0 is the least value (1) of byte 0 is the very first bit consumed.
Otherwise all other operations are on parallel bytes, and endian-ness
should not matter; all final entropy reads are done in byte sized units
anyhow; so bit order isn't really a consideration.

## Overview

Basically, this takes an input, adds one byte to the length, and pads
to 8 byte boundary (int64); the length of the pad is stored in the last
byte; unused padding bytes will be set to 0.  

Summary; again

   - Simple xor-chains a 256 bit mask computed from the key using KangarooTwelve(K12) over the data.
   - swap all bytes for some other byte through a reversible map shuffled using the input key as the seed.
   - xor 0x55 into first byte, store that result
       - swap byte for another byte
       - use that result to xor on the next byte, repeatedly.
   - xor 0xAA into last byte, store that result, use that result to xor on the previous byte, repeatedly.
   - swap all bytes for some other byte
 
 
Data is processed in blocks of 4096 bytes.  The last block of data is processed only 8-4096 bytes (of
only 8 byte length units).  This allows multiple sections of large data to be processed in parallel.
A 1 bit change in the plain text will only affect the 4096 byte block it is in; but will on average
cause 16384 bits to change in the output.


```
encrypt( data, datalen ) { 
  int outlen = datalen + 1; // add 1 byte for pad length
  if( outlen & 7 )
	outlen += ( 8 - (outlen&7) );
  char *output = malloc( outlen );
  ((uin64_t*)( output + outlen-8 ))[0] = 0; // make sure all extra pad bytes are 0
  output[outlen-1] = outlen - datalen; // save pad to recover length.
  memcpy( output, data, datalen );
  /* ... do real work ... */
}
```
 
## Algorithm Setup

The, current, strongest, fastest setup is done with K12.  Internally there is
support to change the CSPRNG; (SHA[1/2_256/2_512/3]/k12).

The algorithm performs quite a bit of setup before applying the transform
to data. There is a mask generated (basically) with 256 bits from K12(key data).

The same K12 hash generator is used from the initial seeding of the specified key information
is used for the mask and byte subtitution mapping creation.

K12.Init()
K12.Update( key );
K12.Final();
mask_entropy = K12.Squeeze( 256 );     // bits
shuffle_entropy = K12.Squeeze( 256*8 );  // bits





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
    Ax-Hx = plain text ^ mask(0-n)
    A-H   = Ax-Hx xboxSub
    5     = A ^ 0x55    
    0     = H' ^ 0xAA      //  (10 = 0)
    A'-H' = each byte xor next byte from first with first = 0x55
    A"-H" = each byte xor prev byte from last  with last = 0xAA
	 	
		
given     a b c d   e f g h 
         
       ^  CiphA-D   CiphE-H     apply mask to plaintext
	     
       =  AcBcCcDc  EcFcGcHc    masked text
	     
 xboxSub     - xbox -           masked text swapped through xbox
         
         
       =  A B C D   E F G H     a^k, b^k,...
       ^  5 A B C   D E F G
       ^    5 A B   C D E F
       ^      5 A   B C D E
       ^        5   A B C D
       ^            5 A B C
       ^              5 A B
       ^                5 A
       ^                  5 

 xboxSub     - xbox -           xor'd sum then byte swapped through xbox
         
       =  A'B'C'D'  E'F'G'H'    above columns xor'd together


	   
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

	for( n = 0; n < outlen; n += 8, output += 8 )
		((uint64_t*)output)[0] ^= ((uint64_t*)(bufKey + (n % 32)))[0];
	   
	BlockShuffle_SubBytes_( bytKey, output, output, outlen );

	for( n = 0, p = 0x55; n < outlen; n++, output++ ) 
		p = output[0] = BlockShuffle_SubByte_( output[0] ^ p );

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
		curBuf[0] = BlockShuffle_BusByte_( curBuf[0] ) ^ curBuf[1];
	}
	curBuf[0] = curBuf[0] ^ 0xAA;	

	curBuf = output + len - 1;
	for( n = (len - 1); n > 0; n--, curBuf-- ) {
		curBuf[0] = curBuf[0] ^ curBuf[-1];
	}
	curBuf[0] = curBuf[0] ^ 0x55;

	BlockShuffle_BusBytes_( bytKey, output, output, len );

	for( n = 0; n < len; n += 8, output += 8 ) {
		((uint64_t*)output)[0] ^= ((uint64_t*)(bufKey + (n % (RNGHASH / 8))))[0];;
	}
	   
```


## xbox generation

The XBox is a unique 2way mapping of input A to output B, and B to A.  This
is done with 1 512 byte array.

This uses 256*8 bits of entropy and does a quick in-place swapping algorithm
to shuffle the bytes.  The first 256 bits of entropy are used for masking the
blocks initially, followed by these.

This is done with the same random_context as the source hash; very improbable
this could be less-than-random.


```
struct byte_shuffle_key {
    uint8_t map[256], dmap[256];
};

struct byte_shuffle_key *BlockShuffle_ByteShuffler( struct random_context *ctx ) {
	struct byte_shuffle_key *key = New( struct byte_shuffle_key );
	int n;
	int srcMap;
	for( n = 0; n < 256; n++ )
		key->map[n] = n;

	// simple-in-place shuffler.
	for( n = 0; n < 256; n++ ) {
		int m;
		int t;
		SRG_GetByte_( m, ctx );
		t = key->map[m];
		key->map[m] = key->map[n];
		key->map[n] = t;
	}
	for( n = 0; n < 256; n++ )
		key->dmap[key->map[n]] = n;
	return key;
}
```

---

# Historical and development notes

Disregard from here down.


---

byte permuations

2^256 ...(ish)
116,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000.

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
Tiny DID  300000 in 1257    238663    10,501,172
Tiny DID  900000 in 3437    261856    11,521,664
Big DID   300000 in 4171     71925   147,302,400
Big DID   300000 in 3239     92621   189,687,808
Mega DID     300 in 4956        60   251,658,240
Mega DID     300 in 6490        46   192,937,984

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


Tiny DID 900000 in 3778   238221  10481724
Big DID  300000 in 3493    85886 175894528
Mega DID    300 in 4481       66 276824064

Tiny DID 900000 in 3502   256996  11307824
Big DID  300000 in 2545   117878 241414144
Mega DID    300 in 2585      116 486539264

Tiny DID 900000 in 3510   256410  11,282,040
Big DID  300000 in 2371   126528 259,129,344
Mega DID    300 in 2419      124 520,093,696


Tiny DID 900000 in 3837   234558  10,320,552
Big  DID 300000 in 2860   104895 214,824,960
Mega DID    300 in 3066       97 406,847,488
DID 300000 in 1151   260642


Tiny DID 900000 in 4756   189234   8326296
Big  DID 300000 in 3249    92336 189104128
Mega DID    300 in 3228       92 385875968


Tiny DID 900000 in 2324   387263 17039572
Big DID 300000 in 2756   108853 222930944
Mega DID 300 in 3493   85 356515840



## xbox generation (small-entropy)

This is a modified version of the above, it uses only approximately 112 bits of 
entropy to shuffle the deck.  This causes fewer hashes generated internally.


Pseudo-Random Bit (PRB) is a single bit from the entropy generating hash
function.  

The shuffle algorithm was implemented in such a way as to use only a few
bits from the entropy stream.  Other solutions used (8*256; 2K) bits, which
caused a heavier load on the hash generation.

The range of bytes is considered as a deck of cards.

In operation, the deck of cards is split into two equal stacks (128 each).
Each of these stack is then split into 3 ranges of 43 or 42 cards each.

The stacks are shuffled 5 times.
FOr each round, 2-PRB are used to determine the order of the left stacks,
another 2 PRB are used to determine the order of the right stacks. And 
1 PRB is picked to determine to start with left or right stack.

(25 bits)

1 PRB is picked to determine starting to... control a maximum card stack
length before forcing a swap.
(26 bits)

Then, ordering for as many as 86 cards is chosen.  Take one card from
one stack; get 1 PRB, if (1), take another card from the same stack; else
set the count of cards to take into the counter-list; and reset count to 1.
Repeat getting 1PRB, until enough cards are matched.  As many as 86 bits 
are used; THere is s small probability that enough cards have been counted
that the target stack is full, and just the remaining cards needs to be set 
on the other side. Additionally, if more than (4 or 5) cards in a row are 
picked, force saving that count, and 'switch stacks'.  The 1 PRB picked
earlier determines whether the limit is 4 or 5 cards, which is sort-of 
mapped to left/right stacks,  but depending on that bit, and the bit 
chosen for the shuffle round left/right swap.  

It's obvious, from counting the bits of entropy used, there is a maximum 
of 2^112 different outcomes.  Ideal permutations of 256 cards is factorial(256).


```
	for( n = 0; (t[0] < 43 || t[1] < 43 ) && n < 86; n++ ) {
		int bit;
		int c;
		c = 1;
		while( c < (5- lrStart) && ( SRG_GetBit_( bit, ctx ), !bit ) ) {
			c++;
		}
		lrStart = !lrStart;
		stacks[n] = c;
		t[n&1] += c;
	}
```


### Example Configurations


```

//0, 43, 86 
//128, 171, 214

leftStacks [3][2] = { {   0, 43 }, { 43, 43}, { 86,42} };
rightStacks[4][2] = { { 128, 43 }, {171, 43}, {214,42} };

leftOrders [4][3] = { { 1, 0, 2 }, { 1, 2, 0 }, {2, 1, 0 }, {2, 0, 1 } };
rightOrders[4][3] = { { 0, 2, 1 }, { 2, 0, 1 }, { 1, 2, 0 }, {2, 1, 0 } };

struct halfDeck {
	int from;
	int until;
	int cut;
	uint8_t starts[3];
	uint8_t lens[3];
};

struct byte_shuffle_key *BlockShuffle_ByteShuffler( struct random_context *ctx ) {
	struct byte_shuffle_key *key = New( struct byte_shuffle_key );
	int n;
#define BLOCKSHUF_BYTE_ROUNDS 5
	uint8_t stacks[86];
	uint8_t halves[8][2];
	uint8_t lrStarts[8];
	uint8_t lrStart;
	uint8_t *readLMap;
	uint8_t *readRMap;
	uint8_t *writeMap;
	key->map = NewArray( uint8_t, 512 );
	int srcMap;

	key->dmap = key->map + 256;
	uint8_t *maps[2] = { key->dmap, key->map };
	key->ctx = ctx;

	/* 40 bits for 8 shuffles. */
	for( n = 0; n < BLOCKSHUF_BYTE_ROUNDS; n++ ) {
		halves[n][0] = SRG_GetEntropy( ctx, 2, 0 );
		halves[n][1] = SRG_GetEntropy( ctx, 2, 0 );
		lrStarts[n] = SRG_GetEntropy( ctx, 1, 0 );
	}

	int t[2] = { 0, 0 };
	SRG_GetBit_( lrStart, ctx );
	for( n = 0; (t[0] < 43 || t[1] < 43 ) && n < 86; n++ ) {
		int bit;
		int c;
		c = 1;
		while( c < (5- lrStart) && ( SRG_GetBit_( bit, ctx ), !bit ) ) {
			c++;
		}
		lrStart = !lrStart;
		stacks[n] = c;
		t[n&1] += c;
	}
	for( n = 0; n < 256; n++ )
		key->map[n] = n;
	srcMap = 1;
	for( n = 0; n < BLOCKSHUF_BYTE_ROUNDS; n++ ) {
		struct halfDeck left, right;
		int s;
		int useCards;

		left.starts[0] = leftStacks[ leftOrders[ halves[n][0] ] [0] ] [0];
		left.lens[0]   = leftStacks[ leftOrders[ halves[n][0] ] [0] ] [1];
		left.starts[1] = leftStacks[ leftOrders[ halves[n][0] ] [1] ] [0];
		left.lens[1]   = leftStacks[ leftOrders[ halves[n][0] ] [1] ] [1];
		left.starts[2] = leftStacks[ leftOrders[ halves[n][0] ] [2] ] [0];
		left.lens[2]   = leftStacks[ leftOrders[ halves[n][0] ] [2] ] [1];
		left.cut = 0;
		left.from = left.starts[left.cut];
		left.until = left.starts[left.cut] + left.lens[left.cut];

		right.starts[0] = rightStacks[ rightOrders[ halves[n][1] ] [0] ] [0];
		right.lens[0]   = rightStacks[ rightOrders[ halves[n][1] ] [0] ] [1];
		right.starts[1] = rightStacks[ rightOrders[ halves[n][1] ] [1] ] [0];
		right.lens[1]   = rightStacks[ rightOrders[ halves[n][1] ] [1] ] [1];
		right.starts[2] = rightStacks[ rightOrders[ halves[n][1] ] [2] ] [0];
		right.lens[2]   = rightStacks[ rightOrders[ halves[n][1] ] [2] ] [1];
		right.cut = 0;
		right.from = right.starts[right.cut];
		right.until = right.starts[right.cut] + right.lens[right.cut];

		lrStart = lrStarts[n];
		useCards = stacks[s=0];

		readLMap = maps[srcMap] + left.from;
		readRMap = maps[srcMap] + right.from;
		writeMap = maps[1 - srcMap];
		s = 0;
		for( int outCard = 0; outCard < 256;  ) {
			useCards = stacks[s];
			for( int c = 0; c < useCards; c++ ) {
				if( lrStart ) {
					(writeMap++)[0] = (readLMap++)[0];
					outCard++;
					left.from++;
					//maps[1 - srcMap][outCard++] = maps[srcMap][left.from++];
					if( left.from >= left.until ) {
						if( ++left.cut < 3 ) {
							s = 0;
							useCards = stacks[s];
							c = -1;

							left.from = left.starts[left.cut];
							left.until = left.starts[left.cut] + left.lens[left.cut];
							readLMap = maps[srcMap] + left.from;
						}
						while( left.cut != right.cut ) {
							(writeMap++)[0] = (readRMap++)[0];
							outCard++;
							right.from++;
							//maps[1 - srcMap][outCard++] = maps[srcMap][right.from++];
							if( right.from >= right.until ) {
								if( ++right.cut < 3 ) {
									right.from = right.starts[right.cut];
									right.until = right.starts[right.cut] + right.lens[right.cut];
									readRMap = maps[srcMap] + right.from;
								}
							}
						}
						if( s ) break;
						// L/R 2 new stacks... lrStart = same for whole stack each 3 subpart so...;
					}
				}
				else {
					(writeMap++)[0] = (readRMap++)[0];
					outCard++;
					right.from++;
					//maps[1 - srcMap][outCard++] = maps[srcMap][right.from++];
					if( right.from >= right.until ) {
						if( ++right.cut < 3 ) {
							s = 0;
							useCards = stacks[s];
							c = -1;

							right.from = right.starts[right.cut];
							right.until = right.starts[right.cut] + right.lens[right.cut];
							readRMap = maps[srcMap] + right.from;
						}
						while( left.cut != right.cut ) {
							(writeMap++)[0] = (readLMap++)[0];
							outCard++;
							left.from++;
							//maps[1 - srcMap][outCard++] = maps[srcMap][left.from++];
							if( left.from >= left.until ) {
								if( ++left.cut < 3 ) {
									left.from = left.starts[left.cut];
									left.until = left.starts[left.cut] + left.lens[left.cut];
									readLMap = maps[srcMap] + left.from;
								}
							}
						}
						if( s ) break;
						// L/R 2 new stacks... lrStart = same for whole stack each 3 subpart so...;
					}

				}
			}
			if( outCard >= 256 )
				break;
			lrStart = 1-lrStart;
			s++;
			if( s >= 86 ) {
				useCards = stacks[s=0];
			}
		}

		srcMap = 1 - srcMap;
	}

#if 0
    /* original shuffler */
	{
		for( n = 0; n < 256; n++ ) {
			int m = SRG_GetEntropy( ctx, 8, 0 );
			int t = key->map[m];
			key->map[m] = key->map[n];
			key->map[n] = t;
		}

		//ShuffleBytes( key, key->map, 256 );
	}
#endif
	for( n = 0; n < 256; n++ )
		key->dmap[key->map[n]] = n;
	return key;
}

void BlockShuffle_SubByte( struct byte_shuffle_key *key
	, uint8_t *bytes_input, uint8_t *bytes_output ) {
	bytes_output[0] = key->map[bytes_input[0]];
}

void BlockShuffle_BusByte( struct byte_shuffle_key *key
	, uint8_t *bytes_input, uint8_t *bytes_output ) {
	bytes_output[0] = key->dmap[bytes_input[0]];
}

```