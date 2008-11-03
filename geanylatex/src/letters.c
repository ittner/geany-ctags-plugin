/*
 * 		letters.h
 *
 *      Copyright 2008 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include <gtk/gtk.h>
#include "support.h"
#include "datatypes.h"
#include "letters.h"

enum
{
	GREEK_LETTERS = 0,
	GERMAN_LETTERS,
	MISC_LETTERS,
	ARROW_CHAR,
	RELATIONAL_SIGNS,
	BINARY_OPERATIONS,
	LETTERS_END
};

CategoryName cat_names[] = {
	{ GREEK_LETTERS, N_("Greek letters"), TRUE},
	{ GERMAN_LETTERS, N_("German umlauts"), TRUE},
	{ MISC_LETTERS, N_("Misc"), FALSE},
	{ ARROW_CHAR, N_("Arrow characters"), FALSE},
	{ RELATIONAL_SIGNS, N_("Relational"), FALSE},
	{ BINARY_OPERATIONS, N_("Binary operation"), FALSE},
	{ 0, NULL, FALSE}
};

/* Entries need to be sorted by categorie (1st field) or some random
 * features will occure.
 * AAABBBCCC is valid
 * AAACCCBBB is valid
 * ACABCBACB is _not_ valid and will course trouble */
SubMenuTemplate char_array[] = {
	// Greek characters
	{GREEK_LETTERS, "Α", "\\Alpha" },
	{GREEK_LETTERS, "α", "\\alpha" },
	{GREEK_LETTERS, "Β", "\\Beta" },
	{GREEK_LETTERS, "β", "\\beta" },
	{GREEK_LETTERS, "Γ", "\\Gamma" },
	{GREEK_LETTERS, "γ", "\\gamma" },
	{GREEK_LETTERS, "Δ", "\\Delta" },
	{GREEK_LETTERS, "δ", "\\Delta" },
	{GREEK_LETTERS, "δ", "\\delta" },
	{GREEK_LETTERS, "Ε", "\\Epsilon" },
	{GREEK_LETTERS, "ε", "\\epsilon" },
	{GREEK_LETTERS, "Ζ", "\\Zeta" },
	{GREEK_LETTERS, "ζ", "\\zeta" },
	{GREEK_LETTERS, "Η", "\\Eta" },
	{GREEK_LETTERS, "η", "\\eta" },
	{GREEK_LETTERS, "Θ", "\\Theta" },
	{GREEK_LETTERS, "θ", "\\theta" },
	{GREEK_LETTERS, "Ι", "\\Iota" },
	{GREEK_LETTERS, "ι", "\\iota" },
	{GREEK_LETTERS, "Κ", "\\Kappa" },
	{GREEK_LETTERS, "κ", "\\kappa" },
	{GREEK_LETTERS, "Λ", "\\Lambda" },
	{GREEK_LETTERS, "λ", "\\lambda" },
	{GREEK_LETTERS, "Μ", "\\Mu" },
	{GREEK_LETTERS, "μ", "\\mu" },
	{GREEK_LETTERS, "Ν", "\\Nu" },
	{GREEK_LETTERS, "ν", "\\nu" },
	{GREEK_LETTERS, "Ξ", "\\Xi" },
	{GREEK_LETTERS, "ξ", "\\xi" },
	{GREEK_LETTERS, "Ο", "\\Omicron" },
	{GREEK_LETTERS, "ο", "\\omicron" },
	{GREEK_LETTERS, "Π", "\\Pi" },
	{GREEK_LETTERS, "π", "\\pi" },
	{GREEK_LETTERS, "Ρ", "\\Rho" },
	{GREEK_LETTERS, "ρ", "\\rho" },
	{GREEK_LETTERS, "Σ", "\\Sigma" },
	{GREEK_LETTERS, "ς", "\\sigmaf" },
	{GREEK_LETTERS, "σ", "\\sigma" },
	{GREEK_LETTERS, "Τ", "\\Tau" },
	{GREEK_LETTERS, "τ", "\\tau" },
	{GREEK_LETTERS, "Υ", "\\Upsilon" },
	{GREEK_LETTERS, "υ", "\\upsilon" },
	{GREEK_LETTERS, "Φ", "\\Phi" },
	{GREEK_LETTERS, "φ", "\\phi" },
	{GREEK_LETTERS, "Χ", "\\Chi" },
	{GREEK_LETTERS, "χ", "\\chi" },
	{GREEK_LETTERS, "Ψ", "\\Psi" },
	{GREEK_LETTERS, "ψ", "\\psi" },
	{GREEK_LETTERS, "Ω", "\\Omega" },
	{GREEK_LETTERS, "ω", "\\omega" },
	{GREEK_LETTERS, "ϑ", "\\thetasym" },
	{GREEK_LETTERS, "ϒ", "\\upsih" },
	{GREEK_LETTERS, "ϖ", "\\piv" },

	// German Umlaute
	{GERMAN_LETTERS, "ä","\"a"},
	{GERMAN_LETTERS, "ü","\"u"},
	{GERMAN_LETTERS, "ö","\"o"},
	{GERMAN_LETTERS, "ß","\"s"},

	// Czech characters
	{MISC_LETTERS, "ě","\\v{e}"},
	{MISC_LETTERS, "š","\\v{s}"},
	{MISC_LETTERS, "č","\\v[c}"},
	{MISC_LETTERS, "ř","\\v{r}"},
	{MISC_LETTERS, "ž","\\v{z}"},
	{MISC_LETTERS, "ů","\\r{u}"},
	{MISC_LETTERS, "Ě","\\v{E}"},
	{MISC_LETTERS, "Š","\\v{S}"},
	{MISC_LETTERS, "Č","\\v{C}"},
	{MISC_LETTERS, "Ř","\\v{R}"},
	{MISC_LETTERS, "Ž","\\v{Z}"},

	//// Misc
	{MISC_LETTERS, "\\","\\backslash"},
	{MISC_LETTERS, "€", "\\euro"},
	{ARROW_CHAR, "←", "\\leftarrow" },
	{ARROW_CHAR, "↑", "\\uparrow" },
	{ARROW_CHAR, "→", "\\rightarrow" },
	{ARROW_CHAR, "↓", "\\downarrow" },
	{ARROW_CHAR, "↔", "\\leftrightarrow" },
	{ARROW_CHAR, "⇐", "\\Leftarrow" },
	{ARROW_CHAR, "⇑", "\\Uparrow" },
	{ARROW_CHAR, "⇒", "\\Rightarrow" },
	{ARROW_CHAR, "⇓", "\\Downarrow" },
	{ARROW_CHAR, "⇔", "\\Leftrightarrow" },
	{RELATIONAL_SIGNS, "\u2264", "\\leq"},
	{RELATIONAL_SIGNS, "\u2265", "\\geq"},
	{RELATIONAL_SIGNS, "\u220E", "\\qed"},
	{RELATIONAL_SIGNS, "\u2261", "\\equiv"},
	{RELATIONAL_SIGNS, "\u22A7", "\\models"},
	{RELATIONAL_SIGNS, "\u227A", "\\prec"},
	{RELATIONAL_SIGNS, "\u227B", "\\succ"},
	{RELATIONAL_SIGNS, "\u223C", "\\sim"},
	{RELATIONAL_SIGNS, "\u27C2", "\\perp"},
	{RELATIONAL_SIGNS, "\u2AAF", "\\preceq"},
	{RELATIONAL_SIGNS, "\u2AB0", "\\succeq"},
	{RELATIONAL_SIGNS, "\u2243", "\\simeq"},
	{RELATIONAL_SIGNS, "\u2223", "\\mid"},
	{RELATIONAL_SIGNS, "\u226A", "\\ll"},
	{RELATIONAL_SIGNS, "\u226B", "\\gg"},
	{RELATIONAL_SIGNS, "\u224D", "\\asymp"},
	{RELATIONAL_SIGNS, "\u2225", "\\parallel"},
	{RELATIONAL_SIGNS, "\u2282", "\\subset"},
	{RELATIONAL_SIGNS, "\u2283", "\\supset"},
	{RELATIONAL_SIGNS, "\u2248", "\\approx"},
	{RELATIONAL_SIGNS, "\u22C8", "\\bowtie"},
	{RELATIONAL_SIGNS, "\u2286", "\\subseteq"},
	{RELATIONAL_SIGNS, "\u2287", "\\supseteq"},
	{RELATIONAL_SIGNS, "\u2245", "\\cong"},
	{RELATIONAL_SIGNS, "\u2A1D", "\\Join"},
	{RELATIONAL_SIGNS, "\u228F", "\\sqsubset"},
	{RELATIONAL_SIGNS, "\u2290", "\\sqsupset"},
	{RELATIONAL_SIGNS, "\u2260", "\\neq"},
	{RELATIONAL_SIGNS, "\u2323", "\\smile"},
	{RELATIONAL_SIGNS, "\u2291", "\\sqsubseteq"},
	{RELATIONAL_SIGNS, "\u2292", "\\sqsupseteq"},
	{RELATIONAL_SIGNS, "\u2250", "\\doteq"},
	{RELATIONAL_SIGNS, "\u2322", "\\frown"},
	{RELATIONAL_SIGNS, "\u2208", "\\in"},
	{RELATIONAL_SIGNS, "\u220B", "\\ni"},
	{RELATIONAL_SIGNS, "\u221D", "\\propto"},
	{RELATIONAL_SIGNS, "\u22A2", "\\vdash"},
	{RELATIONAL_SIGNS, "\u22A3", "\\dashv"},
	{BINARY_OPERATIONS, "\u00B1", "\\pm"},
	{BINARY_OPERATIONS, "\u2213", "\\mp"},
	{BINARY_OPERATIONS, "\u00D7", "\\times"},
	{BINARY_OPERATIONS, "\u00F7", "\\div"},
	{BINARY_OPERATIONS, "\u2217", "\\ast"},
	{BINARY_OPERATIONS, "\u22C6", "\\star"},
	{BINARY_OPERATIONS, "\u2218", "\\circ"},
	{BINARY_OPERATIONS, "\u2219", "\\bullet"},
	{BINARY_OPERATIONS, "\u22C5", "\\cdot"},
	{BINARY_OPERATIONS, "\u2229", "\\cap"},
	{BINARY_OPERATIONS, "\u222A", "\\cup"},
	{BINARY_OPERATIONS, "\u228E", "\\uplus"},
	{BINARY_OPERATIONS, "\u2293", "\\sqcap"},
	{BINARY_OPERATIONS, "\u2228", "\\vee"},
	{BINARY_OPERATIONS, "\u2227", "\\wedge"},
	{BINARY_OPERATIONS, "\u2216", "\\setminus"},
	{BINARY_OPERATIONS, "\u2240", "\\wr"},
	{BINARY_OPERATIONS, "\u22C4", "\\diamond"},
	{BINARY_OPERATIONS, "\u25B3", "\\bigtriangleup"},
	{BINARY_OPERATIONS, "\u25BD", "\\bigtriangledown"},
	{BINARY_OPERATIONS, "\u25C1", "\\triangleleft"},
	{BINARY_OPERATIONS, "\u25B7", "\\triangleright"},
	{BINARY_OPERATIONS, "", "\\lhd"},
	{BINARY_OPERATIONS, "", "\\rhd"},
	{BINARY_OPERATIONS, "", "\\unlhd"},
	{BINARY_OPERATIONS, "", "\\unrhd"},
	{BINARY_OPERATIONS, "\u2295", "\\oplus"},
	{BINARY_OPERATIONS, "\u2296", "\\ominus"},
	{BINARY_OPERATIONS, "\u2297", "\\otimes"},
	{BINARY_OPERATIONS, "\u2205", "\\oslash"},
	{BINARY_OPERATIONS, "\u2299", "\\odot"},
	{BINARY_OPERATIONS, "\u25CB", "\\bigcirc"},
	{BINARY_OPERATIONS, "\u2020", "\\dagger"},
	{BINARY_OPERATIONS, "\u2021", "\\ddagger"},
	{BINARY_OPERATIONS, "\u2A3F", "\\amalg"},
	{0, NULL, NULL},

};
