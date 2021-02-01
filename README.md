# HP200LX to REX-3 PIM Data Convertor

_Notes, Feb 2021:_ 

* _small changes to source code so it will compile without errors or warnings using a modern gcc compiler._
* _original readme reformatted to markdown and to reflect new license_

__HP2REX v0.94 beta   23 March 2001__

## Disclaimer:

I do not claim that this program works, nor do I claim it will not
wipe out your HP200LX and REX memory.  So BACKUP everything before you
try this code.

## Acknowledgements:

I want to thank several folks who have been of great assistance in this
work:  

  * __Mack Baggette__, for writing the code to read/write from/to a REX card.
    He has been of invaluable benefit to the palmtop community, and I
    thank him for taking time out of his busy schedule for my little
    REX project.

  * __Holger Lembke__, Philipp Adelt and "Rex Tech".  These guys did the brunt
    of the work early on, when the REX was first out, in figuring out the
    data storage format.  Holger maintains an excellent website that shows
    the results of all this research, and all sorts of neat information
    and links about the REX (http://www.hlembke.de/rex/index.html)

  * __Petr Hlineny__ of the Univ of Toronto - Petr contacted me a few weeks ago
    and shared with me the results of his independent research into the
    REX.  This information both helped me solve the mystery of a few
    remaining bytes. His correspondence also re-invigorated my interest 
    in the HP2REX project, which had been dormant during a recent busy
    spurt in my work and family life.

  * The small group of assistants who volunteered to send me their REX
    data file dumps. This helped me to understand the format of the data.  
    Without your help, I would have never figured it all out. (And I'm
    still making good use of these files working on the other REX sections
    like the CLDR data).


## Files

Files Provided in ZIP Distribution Archive:

  * `README.md`     This text file
  * `UNLICENSE`     License file
  * `rexwr.exe`     REX-3 Transfer Program (courtesy Mack Baggette)
  * `h2r.bat`       Sample Batch File
  * `file.map`      Example contacts .MAP file
  * `file.mdf`      Example MEMO definition file
  * `file.ini`      Example INI file

## Other Files Needed:

  `gdbio.exe`     Extracts Phonebook Data into CDF File
  `adbdump.exe`   Extracts Appointment Book into CDF file

## Input/Output Files Used by the Program 
_(note slight changes to extensions, explained below):_

  * CONTACTS:
    - `file.PDB`      used only by gdbio
    - `file.CDP`      generated by gdbio, read by hp2rex
    - `file.MAP`      generated onetime by gdbio, hand modified for REX-3
                      use (see instructions below)

  * MEMOS - TEXT FILE MODE:
    - `file.MDF`      list of text files to be used as REX-3 MEMOs
  
  * MEMOS - NOTETAKER MODE:
    - `file.CDN`      generated by gdbio

  * Output File:
    - `file.BIN`      REX-3 Binary Image File

  * Optional:
    - `file.INI`      This file may override any or all the above default
                    filenames.  See example .INI file provided for format.

## Filename extension changes: 

The comma-delimited files generated by gdbio will be named with the
extension CDx, where 'x' is a letter representing the HP200LX PIM
application.  For example,

  * `CDA`   Appt book
  * `CDP`   Phonebook
  * `CDN`   Notetaker

## Program Usage:

If you want CONTACTS data transferred, generate .cdp file from your phone 
book file using the following gdbio command: 

`  gdbio c:\_dat\file.pdb /n >file.cdp`

If you want to transfer some MEMO files, prepare a file.MDF using a standard
text editor.  It should only contain one filename per line.  See the example
file.mdf provided.

If you want to transfer a Notetaker file, generate a .cdn file from your
Notetaker file using the following gdbio command:

`  gdbio c:\_dat\file.ndb /n >file.cdn`

If you want to transfer an Appointment Book file, generate a .cda file from
your Appt Book file using the following adbdump command:

`  adbdump -c -i an -q2 c:\_dat\file.adb >file.cda`

(If you want CONTACTS data transferred, you must also have created 
a file.MAP; this is a one-time step and is described ib the next section).

Now generate the REX binary file:

```
       hp2rex {fbase} -[ICMNA]

where: -[ICMNA] options (single argument) as follows:
       I  Use the filename overrides specified in .INI file
       C  process and store CONT data
       M  process and store MEMO data from text files
       N  process and store MEMO data from Notetaker
       A  process and store APPT data
```

Next save the .bin file to your REX card:

`  rexwr file.bin`

See the batch file H2R.BAT for en example of running the program
automatically and updating your REX with one command.  You will need
to edit this file to properly reflect your actual filename(s).

You can just run the program with no arguments to get a brief cheat
sheet explaining the arguments and file names.

## CONTACTS Setup:

Generate a file.MAP using the gdbio command as follows:

`  gdbio file.pdb/s >file.map`

Then using your favorite text editor, edit the file.MAP file to include
an extra column of data.  This extra column indicates which REX data
field each HP daya field will be mapped into, and is placed between the 
first FLD column and the second NAME column.  A list of the REX field 
names and corresponding numbers follows:

```
    REX    
    FLD   REX FIELD
    NUM   DESCRIPTION
    ===   =================
      0   LAST NAME
      1   FIRST NAME
      2   COMPANY
      3   Title
      4   Work Address
      5   Work-Addr2
      6   Work-City
      7   Work-State
      8   Work-Zip
      9   Work-Country
     10   Home-Address
     11   Home-Addr2
     12   Home-City
     13   Home-State
     14   Home-Zip
     15   Home-Country
     16   WORK PHONE
     17   HOME PHONE
     18   EMAIL
     19   WORK FAX
     20   HOME FAX
     21   CELL PHONE
     22   CAR PHONE
     23   PAGER
     24   OTHER
     25   MAIN PHONE
     26   WEB
```
     
For example, consider the .map file that I prepared for my installation.  
You will see the extra column of integers that I inserted with the heading 
of "Rex".  This example will probably work with minimal modofications
for others installations as well.  Note that I have a slightly modified
.PDB file - I have added an e-mail field to it.  If you have a standard
.PDB file, you will have to modify this example slightly.

```
    <BEG EXAMPLE FILE.MAP>
    Fld Rex Name                 Type      Off Reserved     Flags
      1  0  Na&me                string      0              Relative
      2 16  &Business            phone       2              Relative
      3 17  H&ome                phone       4              Relative
      4 24  &Alternate           phone       6              Relative
      5 19  Fa&x                 phone       8              Relative
      6  3  T&itle               string     10              Relative
      7  1  Cate&gory            category   12 CatRec=0     Relative
      8  2  Com&pany             string     14              Relative
      9 18  &e-mail              string     16              Relative
     10  4  Address&1            string     18              Relative
     11  5  Address&2            string     20              Relative
     12  6  &City                string     22              Relative
     13  7  Sta&te               string     24              Relative
     14  8  &Zip                 string     26              Relative
     15 27  &Note                note       28             
    <Section w/Category Names Omitted for convenience>
    <END EXAMPLE FILE.MAP>
```

The REX database has separate fields for FIRST and LAST names.  The
HP200 phonebook app does not - it has only a NAME field.  Therefore,
I chose to store the HP's CATEGORY field as the REX's FIRSTNAME.  This
way one can sort the REX data on category, which may be occassionally
useful.  Note that this current version of the program only stores
one subset, or cardfile, into the REX.  This is what is commonly called
the ALL cardfile.

## MEMO Setup:

If you want to store MEMOs into your REX, use the -m option.  The
program will look for a file.MDF (Memo Description File) and will
scan it for filenames - one per line.  It will then open each file,
and store it into the REX MEMO format.  The first line of each text
file is interpreted by the program to be the title.  Alternatively,
you can give a title in the .MDF file after the filename (space or
tab betweeen them).

You can now also or instead specify MEMOs for your REX using the
Notetaker file.  You make a comma delimited file of your .NDB file
with gdbio, which the program will process when given the -n option.


## FEATURES TO BE ADDED:

- Read HP200 APPT database and store this into REX's CLDR and TODO
  sections.

- Read NoteTaker file(s) in addition to plain text files for the
  MEMO section.

- Add capability to read HP databases directly, thus eliminating the
  need for separate extraction programs. (low priority)

- Write to REX card directly from hp2rex program, eliminating the
  separate step of REXWR.  This would also eliminate to need for a
  256KB intermediate file.


# Revision History

* __0.90__  Initial Beta Release

* __0.91__  Fixed number of HP Phonebook fields, was hardcoded to 15.  Now
      is a variable obtained from your .map file.  Also changed args to
      fopen() of REX output file to update mode.

* __0.91a__ Changed maximum number of contacts from 512 to 800.

* __0.91b__ Changed maaximum number of HP fields from 24 to 36.

* __0.92__  Retain increased limits from previous 091a/b revisions
      Added ability to read Notetaker file 
      Allow filename overrides in .INI file
      Title for text file notes may now be specified in .MDF file

* __0.93__  Preliminary version of Appt book transfers.  Will transfer
      non-repeating appts correctly.  Others just show up incorrectly
      as a single entry.  Since this may be of some value to some
      users, I decided to go ahead and release this interim version.

* __0.94__  Made a minor modification which allows very long lines for the
      MEMO input files.  Previously this was limited to 256 characters.
      Now you can have up to 2048 characters per line.  This limit was
      causing grief for some users.

# To-Do List

1.  Number one requested addition is Appt book transfer.  That will be
in next release.  I actually have it partially working, but have a lot
of details to work through.

2.  Retain user's previously specified REX Preferences and Time Zone
settings.  I think I know how to implement this pretty simply.  Look for
it in the next release.

3.  Retain user name and contact information.  This one is harder.  I
have a few ideas, but no firm plans for it yet.

4.  Retain Time.  Similar to #3.  However, I'm holding out a slight chance
that retaining the user PREF and Timezone settings will prevent the clock
from resetting.
