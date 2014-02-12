

// add a listener for mouse position

canvas.addEventListener("mousedown", getPosition, false);

function getPosition(event)
{
  var x = event.x;
  var y = event.y;

  var canvas = document.getElementById("canvas");

  x -= canvas.offsetLeft;
  y -= canvas.offsetTop;

  //alert("x:" + x + " y:" + y);
} 





myCtl.dispatchEvent = function(sender, e){
 
    if(sender.myEvent)
         sender.myEvent();
 } 


//In onLoad handler
 myCtl.myEvent = Function.createDelegate(this, this.onMyEvent);
 
//Wherever you need the event to be fired
 myCtl.dispatchEvent(sender, args);
 

this.onMyEvent = function(){
     alert("myEvent Fired!");
 } 










•onclick – As its name suggests this event fires when the user clicks
the mouse button once. This is a very useful event to handle everything
from playing a simple sound on a click to a complex situation for
loading a menu based on the location of the pointer when the user
clicks. 

•ondblclick – This event fires on a double click by the user. This
event will not fire simultaneously with the onclick event. Depending on
how rapidly the clicks register in succession determines whether the
onclick or ondblclick event fires. 

•onmousedown – This event fires when the mouse button is pressed. This
event can be used for circumstances where you want to pause an action
while the mouse is pressed or build a script that allows the user to
move objects around the page. However, with the addition of the new
HTML5 drag and drop events (covered later) this event’s role will
likely change. 

•onmouseup – This event fires when the mouse button is released and is
usually used to end whatever action the onmousedown event started. Like
its partner event onmousedown, the role of this event will also change
with the addition of the new drag and drop events (covered later). 

•onmouseover – With this event you can do all kinds of useful things
like changing an image, opening up a menu or kick starting an
animation. Of all the mouse related events this one and its partner
event onmouseout are probably the most widely used. 

•onmouseout – The opposite of onmouseover this event fires when the
user moves off of an element. This is most commonly used to reset or
end an action started in the onmouseover event. 

•onmousemove  – Think of this as the event that happens between
onmouseover and onmouseout. It fires repeatedly as the mouse pointer
moves over an element. Though interesting in concept it is probably the
least used of the HTML4 mouse events. 









onkeydown
 
script 
 
Script to be run when a key is pressed down
 


onkeypress
 
script 
 
Script to be run when a key is pressed and released
 


onkeyup
 
script 
 
Script to be run when a key is released
 










