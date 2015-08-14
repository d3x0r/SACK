package srg

//import "binary"
import "hash"
import "crypto/sha1"
import "crypto/sha512"
//import "fmt"


type EntropyFeeder interface {
	GetSalt() []byte;
}

type Entropy struct {
	use_version2 bool;
	
	digest hash.Hash;
	//d2 *sha512.digest;

	salt []byte
	
	feeder EntropyFeeder;
	
	//void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size );
	//PTRSZVAL psv_user;
	
	entropy []byte;
	
	bits_used uint;
	bits_avail uint;
};

func (ctx *Entropy) needBits( ) {
	
	if ctx.feeder != nil {
		ctx.salt = ctx.feeder.GetSalt();
 	} else {
		ctx.salt = []byte{};
	}
        
       	ctx.digest.Write( ctx.salt );
       	ctx.entropy = ctx.digest.Sum( nil );
       	ctx.digest.Reset();
       	ctx.digest.Write( ctx.entropy );
        
	ctx.bits_avail = uint(len( ctx.entropy ) * 8);
	ctx.bits_used = 0;
}


func New( version int, feeder EntropyFeeder ) *Entropy {
	ctx := Entropy{};
	ctx.use_version2 = (version==2);
	if( ctx.use_version2 ) {
		ctx.digest = sha1.New();
	} else {
		ctx.digest = sha512.New();
	}
	
	ctx.feeder = feeder;
	ctx.bits_used = 0;
	ctx.bits_avail = 0;
	return &ctx;
}


func (ctx *Entropy) GetBuffer( bits uint ) []byte {
	var tmp uint;
	var partial_tmp uint;
	var partial_bits uint  = 0;
	var get_bits uint;
        var n int = 0;
	var result []byte = make( []byte, (bits+7)/8, (bits+7)/8 );
	for ; bits > 0 ;  {
		if bits > 8 {
			get_bits = 8;
		} else {
			get_bits = bits;
		}
		// only greater... if equal just grab the bits.
                //fmt.Println( "getting ", bits, " of bits avail ", ctx.bits_avail, " remaining ", ctx.bits_avail - ctx.bits_used, " ", ctx.bits_used );
		if( get_bits > ( ctx.bits_avail - ctx.bits_used ) ) {
			if ( ctx.bits_avail - ctx.bits_used ) > 0  {
				partial_bits = ctx.bits_avail - ctx.bits_used;
				if partial_bits > 8 {
					partial_bits = 8;
				}
	      		       	partial_tmp = uint(ctx.entropy[(ctx.bits_used)/8]);
				if( ctx.bits_used & 7 != 0 ) {
					partial_tmp >>= ctx.bits_used & 7;
				}
			}
                        // don't bother incrementing bits_used; this gets a new set of bits
			ctx.needBits( );
			bits -= partial_bits;
		} else {
			if( partial_bits > 0 ) {
	       			if( ctx.bits_used & 7 != 0 ) {
		      		       	tmp = ( uint( ctx.entropy[(ctx.bits_used + 8)/8] ) << 8 ) | uint( ctx.entropy[(ctx.bits_used)/8] );
       					tmp >>= ctx.bits_used & 7;
                        	} else {
	      		       		tmp = uint( ctx.entropy[(ctx.bits_used)/8] );
	                        }
				tmp = partial_tmp | ( tmp << partial_bits );
                                partial_bits = 0;
                        } else {
	       			if( ( ctx.bits_avail - ctx.bits_used ) > 8 && ctx.bits_used & 7 != 0 ) {                	                                	
		      		       	tmp = ( uint( ctx.entropy[(ctx.bits_used + 8)/8] ) << 8 ) | uint( ctx.entropy[(ctx.bits_used)/8] );
       					tmp >>= ctx.bits_used & 7;
                        	} else {
	      		       		tmp = uint( ctx.entropy[(ctx.bits_used)/8] );
	                        }
        		}
			ctx.bits_used += get_bits;
                        //fmt.Println( "bits is now ", ctx.bits_used, " incremented by ", get_bits )
			result[n] = byte( tmp );
                        n++;
			bits -= get_bits;
		}
	} 
	return result;
}

func ( ctx *Entropy ) Get( bits uint, get_signed bool ) int {
	if( bits > 30 ) {
        	panic( "srg.Entropy.Get is only valid for a count of bits less than 30; use GetBuffer for more bits" )
        }
	var result uint = 0;
	var n int;
	tmp := ctx.GetBuffer( bits );
	for n = 0; n < len( tmp ); n++ {
		result |= uint( tmp[n] ) << (uint(8) * uint(n));
	}
        result &= ^uint( 0 ) >> ( uint(32) - bits );
	if get_signed {
		if result & ( 1 << ( bits - 1 ) ) != 0 {
			var negone uint = ^uint(0);
			negone <<= bits;
			return int( result | negone );
		}
        }
	return int(result);
}

func ( ctx *Entropy ) GetRange( min int, max int ) int {
	
	var r int = max - min;
        if( r < 0 ) { panic( "srg.Entropy.GetRange max is less than min specified" ); }
        if( r > ( 1 << 29 ) ) { panic( "srg.Entropy.GetRange range is greater than 29 bits allowed" ); }
        var n uint;
        for n = 31; n > 0; n-- {
        	if( ( r & ( 1 << n ) ) != 0 ) {
                	n++
	                break;
                }
        }
        var val int = ctx.Get( uint(n), false );
        return min + ( val % (r+1) );
}

func ( ctx *Entropy ) Reset() {
	ctx.digest.Reset();
	ctx.bits_used = 0;
	ctx.bits_avail = 0;
}

/*
void SRG_SaveState( struct random_context *ctx, POINTER *external_buffer_holder )
{
	if( !(*external_buffer_holder) )
		(*external_buffer_holder) = New( struct random_context );
	MemCpy( (*external_buffer_holder), ctx, sizeof( struct random_context ) );
}

void SRG_RestoreState( struct random_context *ctx, POINTER external_buffer_holder )
{
	MemCpy( ctx, (external_buffer_holder), sizeof( struct random_context ) );
}
*/

