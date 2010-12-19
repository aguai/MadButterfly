#!/usr/bin/env python
from math import pi, sin, cos, sqrt

# #define FRACTION_SHIFT 10
# 
# #define REF_RADIUS_SHIFT 10
# #define SLOPE_TAB_SZ 128
# #define ARC_RADIUS_RATIO_TAB_SZ 128
# #define ARC_RADIUS_FACTOR_TAB_SZ ARC_RADIUS_RATIO_TAB_SZ
# #define SIN_TAB_SZ 256
# static int slope_tab[SLOPE_TAB_SZ];
# static int center_shift_tab[SLOPE_TAB_SZ][2];
# static int vector_len_factor_tab[SLOPE_TAB_SZ];
# static int arc_radius_ratio_tab[ARC_RADIUS_RATIO_TAB_SZ];
# static int arc_radius_factor_tab[ARC_RADIUS_FACTOR_TAB_SZ];
# static int sin_tab[SIN_TAB_SZ];

class tabs_generator(object):
    _fraction_shift = 10
    _ref_radius_shift = 10
    _slope_tab_sz = 128
    _arc_radius_ratio_tab_sz = 256
    _arc_radius_factor_tab_sz = 256
    _sin_tab_sz = 256
    
    def gen_slope_tab(self):
        lines = []
        line = '''\
/*! \\brief The table used to map a slope to an index.
 *
 * The index is used to be a key in other tables.
 * The table is an array of slope values for vectors in 0~(PI/4)
 * direction.
 */\
'''
        lines.append(line)
        line = 'int slope_tab[SLOPE_TAB_SZ] = {'
        lines.append(line)
        
        factor = 1 << self._fraction_shift
        
        for i in range(self._slope_tab_sz):
            angle = pi / 4 * i / (self._slope_tab_sz - 1)
            slope = int(sin(angle) / cos(angle) * factor)
            line = '    %d,' % (slope)
            lines.append(line)
            pass
        
        line = '    };'
        lines.append(line)
        return lines

    def gen_center_shift_tab(self):
        lines = []
        line = '''\
/*! \\brief The table maps the slope of an arc to the factors of shifting.
 *
 * Every mapped slope is associated with two factors for x and y
 * axis respective.  The are multiplied with length of the arc to
 * get shifting value in x and y axis direction.
 */\
'''
        lines.append(line)
        line = 'int center_shift_tab[SLOPE_TAB_SZ][2] = {'
        lines.append(line)

        radius = 1 << (self._ref_radius_shift + self._fraction_shift)

        for i in range(self._slope_tab_sz):
            angle = pi / 4 * i / (self._slope_tab_sz - 1) + pi / 2
            x = int(cos(angle) * radius)
            y = int(sin(angle) * radius)
            line = '    {%d, %d},' % (x, y)
            lines.append(line)
            pass

        line = '    };'
        lines.append(line)
        return lines

    def gen_vector_len_factor_tab(self):
        lines = []
        line = '''\
/*! \\brief The table maps a slope to a lenght factor for a vector.
 *
 * The factor is used to multipled with one of axis values
 * to get the lenght of the vector.
 * The range of mapped slopes are 0~(PI/4).
 */\
'''
        lines.append(line)
        line = 'int vector_len_factor_tab[SLOPE_TAB_SZ] = {'
        lines.append(line)

        frac_factor = 1 << self._fraction_shift

        for i in range(self._slope_tab_sz):
            angle = pi / 4 * i / (self._slope_tab_sz - 1)
            factor = int((1 / cos(angle)) * frac_factor)
            line = '    %d,' % (factor)
            lines.append(line)
            pass

        line = '    };'
        lines.append(line)
        return lines

    def gen_arc_radius_ratio_tab(self):
        lines = []
        line = '''\
/*! \\brief A table of ratio from an arc to its radius.
 *
 * It is to find an index for a given ratio value.
 */\
'''
        lines.append(line)
        line = 'int arc_radius_ratio_tab[ARC_RADIUS_RATIO_TAB_SZ] = {'
        lines.append(line)

        frac_factor = 1 << self._fraction_shift

        for i in range(self._arc_radius_ratio_tab_sz):
            arc_ratio = 2.0 * i / (self._arc_radius_ratio_tab_sz - 1)
            arc_ratio = int(arc_ratio * frac_factor)
            line = '    %d,' % (arc_ratio)
            lines.append(line)
            pass

        line = '    };'
        lines.append(line)
        return lines

    def gen_arc_radius_factor_tab(self):
        lines = []
        line = '''\
/*! \\brief The table maps an arc-radius ratio to a distance factor.
 *
 * The factor is multiplied with radius to get distance of arc and
 * center.  It is in the order of arc_radius_ratio_tab.
 */\
'''
        lines.append(line)
        line = 'int arc_radius_factor_tab[ARC_RADIUS_FACTOR_TAB_SZ] = {'
        lines.append(line)

        frac_factor = 1 << self._fraction_shift

        for i in range(self._arc_radius_factor_tab_sz):
            arc = 2.0 * i / (self._arc_radius_factor_tab_sz - 1)
            factor = int(sqrt(1 - (arc / 2) ** 2) * frac_factor)
            line = '    %d,' % (factor)
            lines.append(line)
            pass

        line = '    };'
        lines.append(line)
        return lines
    
    def gen_sin_tab(self):
        lines = []
        line = '/*! \\brief A table of sin() values */'
        lines.append(line)
        line = 'int sin_tab[SIN_TAB_SZ] = {'
        lines.append(line)

        frac_factor = 1 << self._fraction_shift

        for i in range(self._sin_tab_sz):
            angle = i * pi / 2 / (self._sin_tab_sz - 1)
            _sin = int(sin(angle) * frac_factor)
            line = '    %d,' % (_sin)
            lines.append(line)
            pass

        line = '    };'
        lines.append(line)
        return lines

    def gen_definition(self, out):
        line = '/* This file is generated by tools/gen_precomputed_tabs.py */'
        print >> out, line
        print >> out
        lines = self.gen_slope_tab()
        print >> out, '\n'.join(lines)
        print >> out
        print >> out
        lines = self.gen_center_shift_tab()
        print >> out, '\n'.join(lines)
        print >> out
        print >> out
        lines = self.gen_vector_len_factor_tab()
        print >> out, '\n'.join(lines)
        print >> out
        print >> out
        lines = self.gen_arc_radius_ratio_tab()
        print >> out, '\n'.join(lines)
        print >> out
        print >> out
        lines = self.gen_arc_radius_factor_tab()
        print >> out, '\n'.join(lines)
        print >> out
        print >> out
        lines = self.gen_sin_tab()
        print >> out, '\n'.join(lines)
        print >> out
        pass

    def gen_declaration(self, out):
        line = '''\
#define FRACTION_SHIFT %d

#define REF_RADIUS_SHIFT %d
#define SLOPE_TAB_SZ %d
#define ARC_RADIUS_RATIO_TAB_SZ %d
#define ARC_RADIUS_FACTOR_TAB_SZ %d
#define SIN_TAB_SZ %d

extern int slope_tab[SLOPE_TAB_SZ];
extern int center_shift_tab[SLOPE_TAB_SZ][2];
extern int vector_len_factor_tab[SLOPE_TAB_SZ];
extern int arc_radius_ratio_tab[ARC_RADIUS_RATIO_TAB_SZ];
extern int arc_radius_factor_tab[ARC_RADIUS_FACTOR_TAB_SZ];
extern int sin_tab[SIN_TAB_SZ];
'''
        line = line % (self._fraction_shift, self._ref_radius_shift,
                       self._slope_tab_sz, self._arc_radius_ratio_tab_sz,
                       self._arc_radius_factor_tab_sz, self._sin_tab_sz)
        print >> out, line
        pass
    pass

if __name__ == '__main__':
    import sys

    def usage(progname):
        print >> sys.stderr, 'Usage: %s <C file> <header file>' % (progname)
        sys.exit(255)
        pass

    if len(sys.argv) != 3:
        usage(sys.argv[0])
        pass
    
    cfile = sys.argv[1]
    hfile = sys.argv[2]
    
    gen = tabs_generator()
    
    cout = file(cfile, 'w+')
    print >> cout, '#include "%s"' % (hfile)
    print >> cout
    gen.gen_definition(cout)
    cout.close()

    hout = file(hfile, 'w+')
    sentinel = '__' + hfile.upper().replace('.', '_') + '_'
    print >> hout, '#ifndef %s' % (sentinel)
    print >> hout, '#define %s' % (sentinel)
    print >> hout
    gen.gen_declaration(hout)
    print >> hout, '#endif /* %s */' % (sentinel)
    hout.close()
    pass
