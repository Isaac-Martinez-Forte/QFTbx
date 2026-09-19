# Writes .qft projects for the reproducible design examples of the corpus
# (documentos/Tesis/1-factibilidad-y-verificacion/datos/ejemplos-publicados.md).
# Plant as an expression with parameters; stability as a constant; tracking
# bounds as polynomial systems; controller structure k, z1, p1 (ZPK).
import os, math
S = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(S, 'raw'); os.makedirs(OUT, exist_ok=True)

def pmul(a, b):
    r = [0.0] * (len(a) + len(b) - 1)
    for i, x in enumerate(a):
        for j, y in enumerate(b): r[i + j] += x * y
    return r
def poly(*factors):
    r = [1.0]
    for f in factors: r = pmul(r, f)
    return r
def param(name, nominal, lo=None, hi=None):
    if lo is None:
        return '<parameter><nominal>%s</nominal><uncertain>false</uncertain></parameter>' % nominal
    return ('<parameter><nominal>%s</nominal><uncertain>true</uncertain><name>%s</name><expr>%s</expr>'
            '<range><min>%s</min><max>%s</max></range></parameter>' % (nominal, name, name, lo, hi))
def plist(tag, items):
    return '<%s size="%d">%s</%s>' % (tag, len(items), ''.join(items), tag)
def system(name, num, den, gain):
    # num/den: coefficient lists, highest power first; num == [1] means "numerator size 0" as in the fixtures
    n = [] if num == [1] or num == [1.0] else [param('', c) for c in num]
    d = [param('', c) for c in den]
    return ('<system name="%s"><type id="3"><expression size="0"/>%s%s%s%s</type></system>'
            % (name, plist('numerator', n), plist('denominator', d), param('', gain), param('', 0)))
def spec_tracking(name, sysname, num, den, gain, f0, f1):
    return ('<specification name="%s"><used>true</used><min-frequency>%s</min-frequency><max-frequency>%s</max-frequency>'
            '<constant>false</constant>%s</specification>' % (name, f0, f1, system(sysname, num, den, gain)))
def spec_stability(gamma, f0, f1):
    return ('<specification name="stability"><used>true</used><min-frequency>%s</min-frequency><max-frequency>%s</max-frequency>'
            '<constant>true</constant><magnitude>%s</magnitude></specification>' % (f0, f1, gamma))
UNUSED = '<specification name=""><used>false</used></specification>'

def project(name, title, num_expr, den_expr, num_params, den_params, gain, omega, gamma, TL, TU, k_range=(0.01, 1e8), extra=''):
    """TL/TU = (num, den, gain) coefficient form; gain = (nominal, lo, hi) or fixed value."""
    f0, f1 = min(omega), max(omega)
    if isinstance(gain, tuple): g = param('k', *gain)
    else: g = param('', gain)
    plant = ('<plant name="%s"><type id="0"><expression size="2"><numerator>%s</numerator><denominator>%s</denominator></expression>'
             '%s%s%s%s</type></plant>' % (title, num_expr, den_expr, plist('numerator', num_params), plist('denominator', den_params), g, param('', 0)))
    specs = [spec_tracking('tracking lower', 'TL', *TL, f0, f1) if TL else UNUSED,
             spec_tracking('tracking upper', 'TU', *TU, f0, f1) if TU else UNUSED,
             spec_stability(gamma, f0, f1), UNUSED, UNUSED, UNUSED, UNUSED]
    om = ('<omega><min>%s</min><max>%s</max><point-count>%d</point-count><type>2</type><values>%s </values></omega>'
          % (f0, f1, len(omega), ' '.join(str(w) for w in omega)))
    ctrl = ('<controller name="n3"><type id="1"><expression size="0"/>%s%s%s%s</type></controller>'
            % (plist('numerator', [param('z1', 1, 0.01, 1000)]), plist('denominator', [param('p1', 1, 0.01, 1000)]),
               param('kc', 1, k_range[0], k_range[1]), param('', 0)))
    xml = ('<?xml version="1.0" encoding="UTF-8"?>\n<QFT version="4">\n  <inputs>\n'
           '%s\n<specifications count="7">%s</specifications>\n%s\n%s\n'
           '  </inputs>\n</QFT>\n') % (plant, ''.join(specs), om, ctrl)
    open(os.path.join(OUT, name + '.qft'), 'w').write(xml)
    print('wrote', name)

# --- the DC motor tracking bounds (corpus, sec. 5.0: A = upper with 0.6584, B or C = lower) ---
A = ([1, 30], [1, 4, 19.752], 0.6584)                    # upper
B = ([1], [1, 17, 82, 120], 120)                          # lower
C = ([1], poly([1, 3], [1, 4], [1, 10], [1, 70]), 8400)   # lower (4th order)
dB6 = round(10 ** (6 / 20), 4)

# T3.1 / N5.1 (Tharewal's plant k/(s(s+a)))
project('dcm-k', 'DC motor k/(s(s+a)) (Tharewal 3.1)', '1', 's*(s+a)', [], [param('a', 1, 1, 10)], (1, 1, 10),
        [0.1, 0.5, 1, 15, 100], 1.2, B, A)
# ex2 plant with the 8 frequencies of Purohit 2016 exp. 4.1
project('dcm-ka-w8', 'DC motor k a/(s(s+a)), 8 frequencies (Purohit 4.1)', 'a', 's*(s+a)', [param('a', 1, 1, 10)], [param('a', 1, 1, 10)], (1, 1, 10),
        [0.5, 1, 2, 3, 5, 10, 30, 60], 1.2, B, A)
# T3.2 / A13: fourth-order lower bound
project('dcm-AC', 'DC motor k a/(s(s+a)), bounds A/C (Tharewal 3.2)', 'a', 's*(s+a)', [param('a', 1, 1, 10)], [param('a', 1, 1, 10)], (1, 1, 10),
        [0.5, 1, 2, 10, 30, 60], 1.2, C, A)
# T3.3: first-order bounds, very low frequencies
project('dcm-T33', 'DC motor k/(s(s+a)), bounds 1.5/(s+1.5) and 1/(s+1)^2 (Tharewal 3.3)', '1', 's*(s+a)', [], [param('a', 1, 1, 10)], (1, 1, 10),
        [0.001, 0.0157, 0.2449, 3.8337, 60], 1.2, ([1], [1, 2, 1], 1), ([1], [1, 1.5], 1.5))
# N5.2 aircraft with integrator, 5 parameters, 6 dB
TU_air = (poly([1 / 0.35, 1]), poly([1 / 0.5, 1], [1 / 3, 1]), 1)
TL_air = ([1], poly([1, 1], [1, 1], [1 / 5, 1]), 1)
air_params = [param('a', 0.5, 0.5, 0.75), param('b', 10, 1, 10), param('wn', 6, 5, 6), param('zeta', 0.8, 0.8, 0.9)]
project('aircraft', 'Aircraft k(1+s/a)/(s(1+s/b)(1+2 zeta s/wn+s^2/wn^2)) (Nandkishor 5.2)', '1+s/a', 's*(1+s/b)*(1+2*zeta*s/wn+s^2/wn^2)',
        [air_params[0]], air_params[1:], (2, 0.2, 2), [0.01, 0.05, 0.1, 0.2, 1, 5, 10], dB6, TL_air, TU_air)
# Purohit 4.4: the same aircraft with 22 frequencies (labels fixed by asymptotics)
TL_fl = ([1], poly([1, 1], [1, 1], [1 / 2, 1]), 1)
project('flight22', 'Flight control, 22 frequencies (Purohit 4.4)', '1+s/a', 's*(1+s/b)*(1+2*zeta*s/wn+s^2/wn^2)',
        [air_params[0]], air_params[1:], (2, 0.2, 2),
        [0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 3, 4, 5, 7, 8, 9, 10, 20, 30, 40, 50, 80, 100, 300], dB6, TL_fl, TU_air)
# T3.6 unstable plant, stability only
project('unstable', 'Unstable k(s+a)/(s^2-2.5) (Tharewal 3.6)', 's+a', 's^2-2.5', [param('a', 1, 0.1, 1)], [], (1, 1, 10),
        [0.1, 1, 2, 6, 50], 2.1, None, None)
# T4.4 MSF desalination, 4 parameters
project('msf', 'MSF desalination K(1+T1 s)/((1+T2 s)(1+T3 s)) (Tharewal 4.4)', '1+T1*s', '(1+T2*s)*(1+T3*s)',
        [param('T1', 20.32, 12, 28)], [param('T2', 18.3, 11, 26), param('T3', 7.2, 4, 10)], (54, 32, 76),
        [0.01, 0.098, 0.309, 0.97, 9.558, 30], 1.2, ([1], [1, 13.5, 50.94, 60.44], 60.44), ([16.32, 197.9], [12.12, 64.65, 197.9], 1))
# T6.2 jet engine
project('jet-g12', 'Jet engine 43/(s^3+a s^2+b s), gamma 1.2 instead of 1.001 (Tharewal 6.2)', '1', 's^3+a*s^2+b*s', [], [param('a', 0.0045, 0.004, 0.005), param('b', 0.45, 0.4, 0.5)], 43,
        [0.001, 0.008, 0.06, 0.5], 1.2, ([1], poly([1, 50.38], [1, 13.5], [1, 3.617]), 2460.375), ([1], [1, 5], 5))
# Purohit 4.2 maglev, lower (stable) and upper (unstable)
mag_TL = ([1], [1, 39.76, 354.9, 916.3], 916.3); mag_TU = ([1.722, 68.89], [1, 16.6, 68.89], 1)
mag_w = [0.1, 1, 1.5, 2, 2.5, 3, 3.66, 5.5, 10, 20, 30]
project('maglev-lower', 'Maglev lower k/(s^2+a) (Purohit 4.2)', '1', 's^2+a', [], [param('a', 430.25, 382, 478.5)], (877.5, 811, 944), mag_w, 1.2, mag_TL, mag_TU)
project('maglev-upper', 'Maglev upper k/(s^2-a) (Purohit 4.2)', '1', 's^2-a', [], [param('a', 430.25, 382, 478.5)], (1063.5, 1021, 1106), mag_w, 1.2, mag_TL, mag_TU)
# Purohit 4.3 first order plus delay (Pade)
project('fopdt', 'FOPDT Pade k(1-td s/2)/((s+a)(1+td s/2)) (Purohit 4.3)', '1-td*s/2', '(s+a)*(1+td*s/2)',
        [param('td', 0.1, 0.08, 0.12)], [param('a', 1.5, 1, 2), param('td', 0.1, 0.08, 0.12)], (2, 1, 3),
        [0.1, 0.2, 0.5, 1, 2, 5, 8, 10, 50], 1.2, ([1], [1, 7, 15, 9], 9), ([1], [1, 3.3, 4], 4))
# --- from the articles Isaac provided (12 Sept, documentos/articulos/nuevos) ---
# Chait 1999 uses ex2 with five frequencies (no 2 rad/s)
project('dcm-ka-w5', 'DC motor k a/(s(s+a)), Chait 1999 frequencies', 'a', 's*(s+a)', [param('a', 1, 1, 10)], [param('a', 1, 1, 10)], (1, 1, 10),
        [0.1, 0.5, 1, 15, 100], 1.2, B, A)
# Garcia-Sanz and Guillen 2000: Thompson's aircraft with nine frequencies up to 100
project('aircraft-w9', 'Aircraft, Garcia-Sanz and Guillen 2000 frequencies', '1+s/a', 's*(1+s/b)*(1+2*zeta*s/wn+s^2/wn^2)',
        [air_params[0]], air_params[1:], (2, 0.2, 2), [0.01, 0.05, 0.1, 0.2, 1, 5, 10, 50, 100], dB6, TL_air, TU_air)
# Bryant and Halikias 1995 (Horowitz and Sidi 1972): k a/(s(s+a)), corridor 1/(s+1)^2 .. 1.5/(s+1.5), 23 log-spaced
# frequencies from 0.01 to 428.1; the article states no margin, gamma 1.2 added as Tharewal 3.3 does
project('dcm-hs72', 'DC motor k a/(s(s+a)), Horowitz-Sidi 1972 corridor (Bryant-Halikias 1995)', 'a', 's*(s+a)', [param('a', 1, 1, 10)], [param('a', 1, 1, 10)], (1, 1, 10),
        [round(10 ** (-2 + i * (math.log10(428.1) + 2) / 22), 4) for i in range(23)], 1.2, ([1], [1, 2, 1], 1), ([1], [1, 1.5], 1.5))
