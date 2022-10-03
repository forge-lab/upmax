#!/usr/bin/python
#Title			: pwcnf.py
#Usage			: python pwcnf.py -h
#Author			: pmorvalho
#Date			: October 03, 2022
#Description		: Class for manipulating partition-based partial weighted CNF formulas (PWCNFs). 
#Notes			: Inspired in the WCNF class available on PySAT 0.1.7.DEV19
#Python Version: 3.8.5
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

from pysat.formula import *

class PWCNF(object):
    """
        Class for manipulating partition-based partial weighted CNF formulas (PWCNF). 
        It can be used for creating formulas, reading them from a file, or writing them to a
        file. The ``comment_lead`` parameter can be helpful when one needs to
        parse specific comment lines starting not with character ``c`` but with
        another character or a string.
        :param from_file: a PWCNF filename to read from
        :param from_fp: a file pointer to read from
        :param from_string: a string storing a PPWCNF formula
        :param comment_lead: a list of characters leading comment lines
        :type from_file: str
        :type from_fp: file_pointer
        :type from_string: str
        :type comment_lead: list(str)
    """
    def __init__(self, from_file=None, from_fp=None, from_string=None,
            comment_lead=['c']):
        """
            Constructor.
        """
        self.nv = 0
        self.hard = []
        self.soft = []
        self.wght = []
        self.topw = 1
        self.comments = []
        self.parts = []
        self.parts_wghts = []

        if from_file:
            self.from_file(from_file, comment_lead, compressed_with='use_ext')
        elif from_fp:
            self.from_fp(from_fp, comment_lead)
        elif from_string:
            self.from_string(from_string, comment_lead)


    def from_file(self, fname, comment_lead=['c'], compressed_with='use_ext'):
        """
            Read a PWCNF formula from a file in the DIMACS format. A file name
            is expected as an argument. A default argument is ``comment_lead``
            for parsing comment lines. A given file can be compressed by either
            gzip, bzip2, or lzma.
            :param fname: name of a file to parse.
            :param comment_lead: a list of characters leading comment lines
            :param compressed_with: file compression algorithm
            :type fname: str
            :type comment_lead: list(str)
            :type compressed_with: str
            Note that the ``compressed_with`` parameter can be ``None`` (i.e.
            the file is uncompressed), ``'gzip'``, ``'bzip2'``, ``'lzma'``, or
            ``'use_ext'``. The latter value indicates that compression type
            should be automatically determined based on the file extension.
            Using ``'lzma'`` in Python 2 requires the ``backports.lzma``
            package to be additionally installed.
            Usage example:
            .. code-block:: python
                >>> from pysat.formula import PWCNF
                >>> cnf1 = PWCNF()
                >>> cnf1.from_file('some-file.wcnf.bz2', compressed_with='bzip2')
                >>>
                >>> cnf2 = PWCNF(from_file='another-file.wcnf')
        """

        with FileObject(fname, mode='r', compression=compressed_with) as fobj:
            self.from_fp(fobj.fp, comment_lead)

    def from_fp(self, file_pointer, comment_lead=['c']):
        """
            Read a PWCNF formula from a file pointer. A file pointer should be
            specified as an argument. The only default argument is
            ``comment_lead``, which can be used for parsing specific comment
            lines.
            :param file_pointer: a file pointer to read the formula from.
            :param comment_lead: a list of characters leading comment lines
            :type file_pointer: file pointer
            :type comment_lead: list(str)
            Usage example:
            .. code-block:: python
                >>> with open('some-file.cnf', 'r') as fp:
                ...     cnf1 = PWCNF()
                ...     cnf1.from_fp(fp)
                >>>
                >>> with open('another-file.cnf', 'r') as fp:
                ...     cnf2 = PWCNF(from_fp=fp)
        """

        def parse_wght(string):
            wght = float(string)
            return int(wght) if wght.is_integer() else decimal.Decimal(string)

        def parse_partition(string):
            p = float(string)
            return int(p) 

        
        self.nv = 0
        self.hard = []
        self.soft = []
        self.wght = []
        self.n_parts = 0
        self.parts = []
        self.parts_wghts = []
        self.topw = 1
        self.comments = []
        comment_lead = tuple('p') + tuple(comment_lead)

        # soft clauses with negative weights
        negs = []

        for line in file_pointer:
            line = line.strip()
            if line:
                if line[0] not in comment_lead:
                    items = line.split()[:-1]
                    p, w, cl = parse_partition(items[0]), parse_wght(items[1]), [int(l) for l in items[2:]]
                    self.nv = max([abs(l) for l in cl] + [self.nv])

                    if w <= 0:
                        # this clause has a negative weight
                        # it will be processed later
                        negs.append(tuple([cl, -w, p]))
                    elif w >= self.topw:
                        self.hard.append(cl)
                    else:
                        self.parts[p].append(cl)
                        self.parts_wghts[p].append(w)
                        self.soft.append(cl)
                        self.wght.append(w)
                        
                elif not line.startswith('p pwcnf '):
                    self.comments.append(line)
                else: # expecting the preamble
                    preamble = line.split(' ')
                    if len(preamble) == 6: # preamble should be "p pwcnf nvars nclauses topw n_part"
                        self.topw = parse_wght(preamble[-2])
                        self.n_part = parse_wght(preamble[-1])
                    else: # preamble should be "p pwcnf nvars nclauses n_parts", with topw omitted
                        self.topw = decimal.Decimal('+inf')
                        self.n_part = parse_wght(preamble[-1])
                    for p in range(self.n_part+1):
                        self.parts.append([])
                        self.parts_wghts.append([])
        # if there is any soft clause with negative weight
        # normalize it, i.e. transform into a set of clauses
        # with a positive weight
        if negs:
            self.normalize_negatives(negs)

        # if topw was unspecified and assigned to +infinity,
        # we will assign it to the sum of all soft clause weights plus one
        if type(self.topw) == decimal.Decimal and self.topw.is_infinite():
            self.topw = 1 + sum(self.wght)

    def normalize_negatives(self, negatives):
        """
            Iterate over all soft clauses with negative weights and add their
            negation either as a hard clause or a soft one.
            :param negatives: soft clauses with their negative weights.
            :type negatives: list(list(int))
        """

        for cl, w, p in negatives:
            selv = cl[0]

            # tseitin-encoding the clause if it is not unit-size
            if len(cl) > 1:
                self.nv += 1
                selv = self.nv

                for l in cl:
                    self.hard.append([selv, -l])
                self.hard.append([-selv] + cl)

            # adding the negation of the clause either as hard or soft
            if w >= self.topw:
                self.hard.append([-selv])
            else:
                self.soft.append([-selv])
                self.parts.append([-selv])
                self.parts_wghts[p].append(w)
                self.wght.append(w)

    def from_string(self, string, comment_lead=['c']):
        """
            Read a PWCNF formula from a string. The string should be specified
            as an argument and should be in the DIMACS CNF format. The only
            default argument is ``comment_lead``, which can be used for parsing
            specific comment lines.
            :param string: a string containing the formula in DIMACS.
            :param comment_lead: a list of characters leading comment lines
            :type string: str
            :type comment_lead: list(str)
            Example:
            .. code-block:: python
                >>> from pysat.formula import PWCNF
                >>> cnf1 = PWCNF()
                >>> cnf1.from_string(='p pwcnf 2 2 2\\n 2 -1 2 0\\n1 1 -2 0')
                >>> print(cnf1.hard)
                [[-1, 2]]
                >>> print(cnf1.soft)
                [[1, 2]]
                >>>
                >>> cnf2 = PWCNF(from_string='p pwcnf 3 3 2\\n2 -1 2 0\\n2 -2 3 0\\n1 -3 0\\n')
                >>> print(cnf2.hard)
                [[-1, 2], [-2, 3]]
                >>> print(cnf2.soft)
                [[-3]]
                >>> print(cnf2.nv)
                3
        """

        self.from_fp(StringIO(string), comment_lead)
        
    
