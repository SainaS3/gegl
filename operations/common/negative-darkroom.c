/* This file is an image processing operation for GEGL
* 
* GEGL is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or (at your option) any later version.
*
* GEGL is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
*/

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>
#include <stdio.h>
#ifdef GEGL_PROPERTIES

#include "negative-darkroom/negative-darkroom-curve-enum.c"

property_enum (curve, _("Characteristic curve"),
		NegCurve, neg_curve, 0)
	description(_("Hardcoded characteristic curve and color data"))

property_double (exposure, _("Exposure"), 0.0)
	description(_("Base enlargement exposure"))
	value_range (-10, 10)
	ui_range (-5, 5)

property_double (expC, _("Cyan filter"), 60)
	description(_("Cyan filter compensation for the negative image"))
	value_range (0, 300)
	ui_range (0, 180)

property_double (expM, _("Magenta filter"), 60)
	description(_("Magenta filter compensation for the negative image"))
	value_range (0, 300)
	ui_range (0, 180)

property_double (expY, _("Yellow filter"), 60)
	description(_("Yellow filter compensation for the negative image"))
	value_range (0, 300)
	ui_range (0, 180)

property_boolean (clip, _("Clip base + fog"), TRUE)
	description (_("Clip base + fog to have a pure white output value"))

property_double (boost, _("Density boost"), 1.0)
	description(_("Boost paper density to take advantage of increased dynamic range of a monitor compared to a photographic paper"))
	value_range (0.25, 4)
	ui_range (1, 2)
	ui_gamma (2)
property_double (contrast, _("Contrast boost"), 1.0)
	description(_("Increase contrast for papers with fixed contrast (usually color papers)"))
	value_range (0.25, 4)
	ui_range (0.75, 1.5)
	ui_gamma (2)

property_double (dodge, _("Dodge/burn multiplier"), 1.0)
	description(_("The f-stop of dodge/burn for pure white/black auxiliary input"))
	value_range (-4.0, 4.0)
	ui_range (0, 2)

property_boolean (preflash, _("Enable preflashing"), FALSE)
	description (_("Show preflash controls"))

property_double (flashC, _("Red preflash"), 0)
	description(_("Preflash the negative with red light to reduce contrast of the print"))
	value_range (0, 1)
	ui_meta("visible", "preflash")

property_double (flashM, _("Green preflash"), 0)
	description(_("Preflash the negative with green light to reduce contrast of the print"))
	value_range (0, 1)
	ui_meta("visible", "preflash")

property_double (flashY, _("Blue preflash"), 0)
	description(_("Preflash the negative with blue light to reduce contrast of the print"))
	value_range (0, 1)
	ui_meta("visible", "preflash")

property_boolean (illum, _("Illuminant adjustment"), FALSE)
	description (_("Show illuminant controls"))
property_double (illumX, _("X multiplier"), 0.965)
	description(_("Adjust the X tristimulus value for output"))
	value_range (0.7, 1.3)
	ui_meta("visible", "illum")
property_double (illumZ, _("Z multiplier"), 0.829)
	description(_("Adjust the Z tristimulus value for output"))
	value_range (0.7, 1.3)
	ui_meta("visible", "illum")
#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     negative_darkroom
#define GEGL_OP_C_SOURCE negative-darkroom.c

#define EPSILON 0.00001

#include "gegl-op.h"

// Color space
typedef struct cieXYZ {
	gfloat X;
	gfloat Y;
	gfloat Z;
} cieXYZ;

// RGB Hurter–Driffield characteristic curve

typedef struct HDCurve {
	gfloat *rx;
	gfloat *ry;
	guint   rn;
	gfloat *gx;
	gfloat *gy;
	guint   gn;
	gfloat *bx;
	gfloat *by;
	guint   bn;
	cieXYZ rsens;
	cieXYZ gsens;
	cieXYZ bsens;
	cieXYZ cdens;
	cieXYZ mdens;
	cieXYZ ydens;
} HDCurve;

#include "negative-darkroom/negative-darkroom-curve-enum.h"

static void
prepare (GeglOperation *operation)
{
	const Babl *space = gegl_operation_get_source_space (operation, "input");
	const Babl *fXYZ;
	const Babl *fRGB;

	fXYZ  = babl_format_with_space ("CIE XYZ float", space);
	fRGB = babl_format ("R~G~B~ float");

	gegl_operation_set_format (operation, "input", fXYZ);
	gegl_operation_set_format (operation, "aux", fRGB);
	gegl_operation_set_format (operation, "output", fXYZ);
}

static gfloat
curve_lerp (gfloat * xs, gfloat * ys, guint n, gfloat in)
{
	if (in <= xs[0])
		return(ys[0]);
	for (guint i = 1; i <= n; i++)
	{
		if (in <= xs[i])
		{
			return(ys[i-1] + (in - xs[i-1]) * \
			       ((ys[i] - ys[i-1]) / (xs[i] - xs[i-1])));
		}
	}	
	return(ys[n-1]);
}

static gfloat
array_min (gfloat * x, guint n)
{
	gfloat min = x[0];
	for (guint i = 1; i < n; i++)
	{
		if (x[i] < min)
			min = x[i];
	}
	return(min);
}

static gfloat
array_max (gfloat * x, guint n)
{
	gfloat max = x[0];
	for (guint i = 1; i < n; i++)
	{
		if (x[i] > max)
			max = x[i];
	}
	return(max);
}

static inline gfloat
clampE (gfloat x)
{
	return(x <= EPSILON ? EPSILON : x);
}

static gboolean
process (GeglOperation       *operation,
	 void                *in_buf,
	 void                *aux_buf,
	 void                *out_buf,
	 glong                n_pixels,
	 const GeglRectangle *roi,
	 gint                 level)
{
	GeglProperties *o = GEGL_PROPERTIES (operation);

	gfloat *in   = in_buf;
	gfloat *aux  = aux_buf;
	gfloat *out  = out_buf;

	gfloat Dfogc = 0;
	gfloat Dfogm = 0;
	gfloat Dfogy = 0;

	gfloat rcomp = 0;
	gfloat gcomp = 0;
	gfloat bcomp = 0;

	gfloat r = 0;
	gfloat g = 0;
	gfloat b = 0;

	gfloat x = 0;
	gfloat y = 0;
	gfloat z = 0;

	gfloat exp = pow(2, o->exposure);

	// Calculate base+fog
	if (o->clip)
	{
		Dfogc = array_min(curves[o->curve].ry,
				  curves[o->curve].rn) * o->boost;
		Dfogm = array_min(curves[o->curve].gy,
				  curves[o->curve].gn) * o->boost;
		Dfogy = array_min(curves[o->curve].by,
				  curves[o->curve].bn) * o->boost;
	}

	// Calculate exposure for mid density
	gfloat Dmaxc = array_max(curves[o->curve].ry, curves[o->curve].rn);
	gfloat Dmaxm = array_max(curves[o->curve].gy, curves[o->curve].gn);
	gfloat Dmaxy = array_max(curves[o->curve].by, curves[o->curve].bn);
	gfloat rMid = curve_lerp(curves[o->curve].ry,
				 curves[o->curve].rx,
				 curves[o->curve].rn,
				 Dmaxc / 2);
	gfloat gMid = curve_lerp(curves[o->curve].gy,
				 curves[o->curve].gx,
				 curves[o->curve].gn,
				 Dmaxm / 2);
	gfloat bMid = curve_lerp(curves[o->curve].by,
				 curves[o->curve].bx,
				 curves[o->curve].bn,
				 Dmaxy / 2);

	if (!aux)
	{
		rcomp = pow(2, (-o->expC) / 30);
		gcomp = pow(2, (-o->expM) / 30);
		bcomp = pow(2, (-o->expY) / 30);
	}

	for (glong i = 0; i < n_pixels; i++)
	{
		/*printf("Input XYZ intensity %f %f %f\n", in[0], in[1], in[2]);*/

		// Calculate exposure compensation from global+filter+dodge
		if (aux)
		{
			rcomp = pow(2, ((-o->expC) / 30) -
					(2 * o->dodge * (aux[0] - 0.5)));
			gcomp = pow(2, ((-o->expM) / 30) -
					(2 * o->dodge * (aux[1] - 0.5)));
			bcomp = pow(2, ((-o->expY) / 30) -
					(2 * o->dodge * (aux[2] - 0.5)));
			aux  += 3;
		}

		// Convert to CIERGB primaries for color filter balance
		x =    0.41847*in[0] -   0.15866*in[1] - 0.082835*in[2];
		y =  -0.091169*in[0] +   0.25243*in[1] + 0.015708*in[2];
		z = 0.00092090*in[0] - 0.0025498*in[1] +  0.17860*in[2];

		// Apply preflash
		x += o->flashC / 100;
		y += o->flashM / 100;
		z += o->flashY / 100;

		// Apply color filters and exposure
		x *= rcomp * exp;
		y *= gcomp * exp;
		z *= bcomp * exp;

		// Simulate emulsion spectral sensitivity with
		// sensitivity matrix
		r = clampE(x * curves[o->curve].rsens.X +
			   y * curves[o->curve].rsens.Y +
			   z * curves[o->curve].rsens.Z);
		g = clampE(x * curves[o->curve].gsens.X +
			   y * curves[o->curve].gsens.Y +
			   z * curves[o->curve].gsens.Z);
		b = clampE(x * curves[o->curve].bsens.X +
			   y * curves[o->curve].bsens.Y +
			   z * curves[o->curve].bsens.Z);

		// Scale the emulsion response
		r *= 5000;
		g *= 5000;
		b *= 5000;

		// Logarithmize the input
		r = log10(r);
		g = log10(g);
		b = log10(b);
		/*printf("Logarithmic RGB intensity %f %f %f\n", r, g, b);*/

		// Apply the DH curve
		r = curve_lerp(curves[o->curve].rx,
			       curves[o->curve].ry,
			       curves[o->curve].rn,
			       r);
		g = curve_lerp(curves[o->curve].gx,
			       curves[o->curve].gy,
			       curves[o->curve].gn,
			       g);
		b = curve_lerp(curves[o->curve].bx,
			       curves[o->curve].by,
			       curves[o->curve].bn,
			       b);
		/*printf("Raw RGB density %f %f %f\n", r, g, b);*/

		// Apply density boost
		r *= o->boost;
		g *= o->boost;
		b *= o->boost;

		// Compensate for fog
		r -= Dfogc;
		g -= Dfogm;
		b -= Dfogy;
		/*printf("Adjusted RGB density %f %f %f\n", r, g, b);*/

		// Adjust contrast
		r = (r - rMid) * o->contrast + rMid;
		g = (g - gMid) * o->contrast + gMid;
		b = (b - bMid) * o->contrast + bMid;

		// Simulate dye density with exponentiation to get
		// the CIEXYZ transmittance back
		out[0] = pow(10, -(r * curves[o->curve].cdens.X +
				   g * curves[o->curve].mdens.X +
				   b * curves[o->curve].ydens.X)) * o->illumX;
		out[1] = pow(10, -(r * curves[o->curve].cdens.Y +
				   g * curves[o->curve].mdens.Y +
				   b * curves[o->curve].ydens.Y));
		out[2] = pow(10, -(r * curves[o->curve].cdens.Z +
				   g * curves[o->curve].mdens.Z +
				   b * curves[o->curve].ydens.Z)) * o->illumZ;
		/*printf("XYZ output %f %f %f\n", out[0], out[1], out[2]);*/

		in   += 3;
		out  += 3;
	}

	return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
	GeglOperationClass               *operation_class;
	GeglOperationPointComposerClass *point_composer_class;

	operation_class      = GEGL_OPERATION_CLASS (klass);
	point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

	operation_class->prepare = prepare;
	operation_class->threaded = TRUE;
	operation_class->opencl_support = FALSE;

	point_composer_class->process = process;

	gegl_operation_class_set_keys (operation_class,
		"name",           "gegl:negative-darkroom",
		"title",          _("Negative Darkroom"),
		"categories",     "color",
		"reference-hash", "2c048959d4827d580d1ab1b6635bc40b",
		"description",    _("Simulate a negative film enlargement in "
				  "an analog darkroom."),
		NULL);
}

#endif

