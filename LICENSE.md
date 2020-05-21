SACK - System Abstraction Component Kit

This project provides abstraction components for windows/linux/BSD/...

All sources from this point 'down' (that is a recursive directory 
listing from the directory which contains this file) are 

Copyright (C) 1996-2020++ to James Buckeyne (d3x0r as in github.com/d3x0r). 

The intent of the current chosen liscense is to protect the work as a whole
from being claimed as the work of another; especially without significant
modifications.

Snippets, and code chunks are permissable to repurpose into other code,
with only an awareness of a request that a notation be made where the 
code was originally sourced from.  If the code is significantly 
modified, please do indicate such, so the original work is not blamed.  
(But; what can I know, I wonder how many people actually read THIS file;
I certainly can't blame you if you were not aware I want to make a request )

'Significant changes' would be... more than ... N.  lines added or deleted.
I don't know how much more you would add or extend this in your own ways...
again; the point isn't to prevent you from using this in any way you wish, 
except to claim this work, as a whole, as it is, as property of someone else.

A good gray example would be, someone refactors the code to remove all
types that are ALL CAPITAL LETTERS.  This change would cause significant
additions and subtractions to code; but it would still essentially be the same...

But, failling  this is liscensed currently as [WTFPL](WTFPL.md).


## Pre 2020/05/20
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; version 2.1
    of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    The LGPL is available at http://www.gnu.org/licenses/lgpl.txt
	 and should be a local copy near this notice.
(and there is :) )

## Included copies of external sources

These projects have their own protections; they are included in-whole
because minor modifications were made to make them build more portably.

Modifications which have been made to each original source are available
in git repository history.  Mostly it's a change to build files (or additional
files to hook into SACK's the existing build system)  Otherwise these are kept
as pristine as possible; (I HATE applying patches, so beleive me, the fewer changes
required the better).

Exceptions: 
   jpeg-6b
   pnglib
   zlib
   snmplib 
   freetype2
   GenX (modified for compilation)
   expat (modified for compilation)
   sqlite
   
   These contain their own copyrights - and are publicly available.
   No modifications have been made to these libraries other 
   than those required to build within this environment, makefiles, 
   and minor modifications to correct compilation errors resulting
   from previously unsupported compiler's (restrictions? methods?
   bugs perhaps?  Though often the error compensated for were sound
   and should have been discovered by any other compiler).
   
   
The following exception also applies:

If modifications are made, and are not remitted to the core library for 
inclusion within the LGPL distribution of this library, then all source 
copyrights are revoked, the source of this library may not be publicized either.
References to originating source would be appreciated.  You may use it
yourself and in you and your companies projects, just don't release it as source... 
Meh - someday this will be a dead project and noone will care, so distribute
source freely - if you attempt to obfuscate original authors, that's pretty rude
and would be plagurism(spelled bad) or something.

Updates from the LGPL distribution are allowed, however, if additions to these
libraries, control extensions for example, are not released in source for public 
use, then no other source of this library may be shared.

---------- Revision 2012, Aug 15 ------------------

In yet other words.... I would appreciate additional fixes/features to this core library, 
and will work graciously with you to have them integrated into the mainline.

Proprietary projects might be developed using this core library, and those of course
remain outside the library, and beyond the scope of this commentary/liscensing.

If you do make mods, and keep them private, then all other
sources of this library you must also keep private; you may refer to the original
project indicating 'based on X but no longer X'; the new product becomes essentially
trade secret, along with all sources of this library.


<change>
