# SEXP---(S-expressions)
---
(from http://people.csail.mit.edu/rivest/sexp.html)
---

S-expressions are a data structure for representing complex data. They are a variation on LISP S-expressions. (Lisp was invented by John McCarthy).

We have adapted S-expressions for use in SDSI and SPKI. The work reported here is, however, application-independent. (That is, we will use S-expressions in the SPKI/SDSI work, but have developed it and presented it in an application-independent manner, so that others may most easily consider adopting it for other applications.)

SDSI has been developed by Professors Ronald L. Rivest and Butler Lampson of MIT's Laboratory for Computer Science, members of LCS's Cryptography and Information Security research group.

SDSI research is supported by DARPA contract DABT63-96-C-0018, "Security for Distributed Computer Systems".

SPKI has been developed by Carl Ellison and others in the IETF SPKI working group.

We are currently merging the designs to form SPKI/SDSI 2.0. This design will use S-expressions as the basic data structure.

## References and Documentation

[SEXP 1.0 guide(text)](Sexp.txt)

C code

The following code reads and parses S-expressions. It also prints out the S-expressions in a variety of formats. (Note that most applications will not need anything but the simple canonical and transport formats; the code here is considerably more complex because it also supports the advanced format, both for input and for output.) The "sexp-main" program also contains some documentation on running the program. (Or just type "sexp -h" to get the help print-out.) The code is available under the "MIT License" (open source).

- [sexp.h](sexp.h)
- [sexp-basic.c](sexp-basic.c)
- [sexp-input.c](sexp-input.c)
- [sexp-output.c](sexp-output.c)
- [sexp-main.c](sexp-main.c)

Here are some sample inputs and outputs (warning: while these look like SDSI/SPKI files, they are only approximations).

- canonical (sexp-sample-c)
- transport (sexp-sample-b)
- advanced  (sexp-sample-a)
