cdef extern from "hash.h":
    ctypedef  unsigned long  int  ub4
    ctypedef  unsigned char  ub1
    cpdef ub4 hash(ub1* k, ub4 length, ub4 initval)
