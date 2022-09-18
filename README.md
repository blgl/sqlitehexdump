An extension for SQLite3 that provides a hexdump function for debugging
purposes.  The output format is based on the canonical hex+ASCII option
of the ubiquitous `hexdump` program:

* An output line consists of a hexadecimal byte offset, a space-separated
list of hexadecimal byte values, and the same bytes as text with any values
outside the printable ASCII range replaced by periods.

* A range of output lines each containing the same values as the previous line
is replaced by a single line containing only an asterisk.

* The final line consists of only the hexadecimal blob length and has no
line terminator.

Example: running the query `select hexdump(data) from sqlite_dbpage;`
in a fresh new database will give you a result similar to this:

```
0000  53 51 4C 69 74 65 20 66 6F 72 6D 61 74 20 33 00  SQLite format 3.
0010  10 00 01 01 00 40 20 20 00 00 00 00 00 00 00 01  .....@  ........
0020  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
*
0060  00 00 00 00 0D 00 00 00 00 10 00 00 00 00 00 00  ................
0070  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
*
1000
```

Build it from `hexdump.c` according to
[the docs](https://www.sqlite.org/loadext.html#compiling_a_loadable_extension).

The first argument of the `hexdump` function is the blob you want to examine.

The optional second argument is an integer specifying the number of bytes
per output line (default 16).  A negative value means to order the bytes
in the hex column of each line from right to left, which may make it easier
to interpret little-endian data.  (The ASCII column is always left-to-right.)

