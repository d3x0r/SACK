
var cards = require( './cards.js' );
var poker = require( './poker.js' );


var deck = cards.Deck( "poker", poker.IterateHoldemHand );
console.log( "got ", deck );
var h1 = deck.Hand();
var h2 = deck.Hand();
var h3 = deck.Hand();
var h4 = deck.Hand();
var h5 = deck.Hand();

var table1 = deck.getCardStack( "table" );

deck.shuffle();
for( var n = 0; n < 2; n++ ) {
	deck.dealTo( h1, false );
	deck.dealTo( h2, false );
	deck.dealTo( h3, false );
	deck.dealTo( h4, false );
	deck.dealTo( h5, false );
}
for( var n = 0; n < 3; n++ )
	deck.dealTo( null, true );
        

