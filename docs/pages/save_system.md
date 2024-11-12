# About the save system

- ## File Format
	- ### Save Directory
		A save is a directory organized as follows
		```
		- SaveDir (directory )
			-regionsDir ( directory )
				-r.X.X.ftmc ( file )
				-r.X.X.ftmc ( file )
				...
		```
	- ### Region Directory
		A region directory contains Region files that are named after their position inside the world
	- ### Region file
		A region file is a binary file containing all infos about a region's chunks.  
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


			 
		
- ## Class Architecture

