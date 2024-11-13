# About the save system

- ## File Format
	- ### Save Directory
		A save is a directory organized as follows
		```
		- SaveDir (directory )
			-regionsDir ( directory )
				-r.X.Y.ftmc ( file )
				-r.X.Y```.ftmc ( file )
				...
		```
	- ### Region Directory
		A region directory contains Region files that are named after their position inside the world
	- ### Region file
		A region file is a binary file containing all infos about a region's chunks.  

		To access a chunk's offset you need to get its relative position from the start of the region.  
		So the pos will be in the range [0-31][0-31].  
		Then you do the operation (x * 32 + y) * 8

		- First there is a 8kB Region Header containinig an offset table
			```
			0x00
			[8 bytes] location info
			0x1FFF
			```
		- Location Info
			```
			bytes		[0-1-2-3] |[4-5-6-7]
			content		[location]|[size]
			```
			the location is an offset in 4kB zones from the start of the file
			the size is also in 4kB zones
		- Chunk info
			a chunk info is a zone of 4kB that can repeat itself
		   ```
			bytes    [0-1-2-3][   4   ][ ... ]
			content  [  len  ][ unused][ compressed data  ]

		   ```

		   len can be smaller than 4092, zones are always padded with zero to have a size of 4kB


			 
		
- ## Class Architecture

