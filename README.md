Inspired by 
[other work](https://github.com/lsulibraries/islandora_change_namespace),
This module presents a proof of the concept that while there is no way
 to directly "convert" an islandora object from one content type to another, 
it is entirely possible to create a new object based on an existing one.

Specifically, because perhaps you have ingested many real-world book(-ish) items
 as compounds, this proof of concept demonstrates how a compound object can be 
converted to a book.

`drush -u 1 islandora_compound_convert_item --parent=my:collection --pid=my:145`

Finally, this is a prototype not optimized for anything, not intended for 
anything beyond proof of concept, research, and my future reference...
