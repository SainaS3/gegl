#!/usr/bin/env ruby
# -*- coding: utf-8 -*-

copyright = '
/* !!!! AUTOGENERATED FILE generated by math.rb !!!!!
 *
 * This file is an image processing operation for GEGL
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
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 *
 * !!!! AUTOGENERATED FILE !!!!!
 */'

a = [
      ['add',       'result = input + value', 0.0, '3b665a3c7f3d3aac89c67bd7051c276f'],
      ['subtract',  'result = input - value', 0.0, '964b3d0b0afea081c157fe0251600ba3'],
      ['multiply',  'result = input * value', 1.0, 'c80bb8504f405bb0a5ce2be4fad6af69'],
      ['divide',    'result = value==0.0f?0.0f:input/value', 1.0, 'c3bd84f8a6b2c03a239f3f832597592c'],
      ['gamma',     'result = powf (input, value)', 1.0, '2687ab0395fe31ccc25e2901a43a9c03'],
#     ['threshold', 'result = c>=value?1.0f:0.0f', 0.5],
#     ['invert',    'result = 1.0-c']
    ]

a.each do
    |item|

    name     = item[0] + ''
    filename = name + '.c'

    puts "generating #{filename}"
    file = File.open(filename, 'w')

    name        = item[0]
    capitalized = name.capitalize
    swapcased   = name.swapcase
    formula     = item[1]

    file.write copyright
    file.write "
#include \"config.h\"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_double (value, _(\"Value\"), #{item[2]})
   description(_(\"global value used if aux doesn't contain data\"))
   ui_range (-1.0, 1.0)

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME         #{name}
#define GEGL_OP_C_FILE       \"#{filename}\"

#include \"gegl-op.h\"

#include <math.h>
#ifdef _MSC_VER
#define powf(a,b) ((gfloat)pow(a,b))
#endif


static void prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, \"input\");
  const Babl *space = NULL;
  const Babl *model = NULL;
  if (!format)
    format = gegl_operation_get_source_format (operation, \"aux\");
  if (format)
  {
    model = babl_format_get_model (format);
  }

  if (babl_model_is (model, \"Y\") ||
      babl_model_is (model, \"Y'\") ||
      babl_model_is (model, \"Y~\"))
  {
    format  = babl_format_with_space (\"Y float\", space);
  }
  else if (babl_model_is (model, \"YA\") ||
           babl_model_is (model, \"Y'A\") ||
           babl_model_is (model, \"Y~A\") ||
           babl_model_is (model, \"YaA\") ||
           babl_model_is (model, \"Y'aA\"))
  {
    format  = babl_format_with_space (\"YA float\", space);
  }
  else if (babl_model_is (model, \"RGB\") ||
           babl_model_is (model, \"R'G'B'\") ||
           babl_model_is (model, \"R~G~B~\"))
  {
    format  = babl_format_with_space (\"RGB float\", space);
  }
#if 0
  else if (babl_model_is (model, \"RGBA\") ||
           babl_model_is (model, \"RGB\") ||
           babl_model_is (model, \"R'G'B'A\") ||
           babl_model_is (model, \"R'G'B'\") ||
           babl_model_is (model, \"R~G~B~A\") ||
           babl_model_is (model, \"R~G~B~\") ||
           babl_model_is (model, \"RaGaBaA\") ||
           babl_model_is (model, \"R'aG'aB'aA\"))
  {
    format  = babl_format_with_space (\"RGBA float\", space);
  }
#endif
  else
  {
    format  = babl_format_with_space (\"RGBA float\", space);
  }

  gegl_operation_set_format (operation, \"input\", format);
  gegl_operation_set_format (operation, \"aux\", format);
  gegl_operation_set_format (operation, \"output\", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat * GEGL_ALIGNED in = in_buf;
  gfloat * GEGL_ALIGNED out = out_buf;
  gfloat * GEGL_ALIGNED aux = aux_buf;
  const Babl *format = gegl_operation_get_format (op, \"output\");
  gint    components = babl_format_get_n_components (format);
  gint    alpha      = babl_format_has_alpha (format);
  gint    i;

  if (aux == NULL)
    {
      gfloat value = GEGL_PROPERTIES (op)->value;
      for (i=0; i<n_pixels; i++)
        {
          gint   j;
          for (j=0; j<components-alpha; j++)
            {
              gfloat result;
              gfloat input=in[j];
              #{formula};
              out[j]=result;
            }
          if (alpha)
            out[components-1]=in[components-1];
          in += components;
          out+= components;
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          gint   j;
          gfloat value;
          for (j=0; j<components-alpha; j++)
            {
              gfloat input =in[j];
              gfloat result;
              value=aux[j];
              #{formula};
              out[j]=result;
            }
          if (alpha)
            out[components-1]=in[components-1];

          in  += components;
          aux += components;
          out += components;
        }
    }
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  point_composer_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  point_composer_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
  \"name\"        , \"gegl:#{name}\",
  \"title\"       , \"#{name.capitalize}\",
  \"categories\"  , \"compositors:math\",
  \"reference-hash\"  , \"#{item[3]}\",
  \"description\" ,
       _(\"Math operation #{name}, performs the operation per pixel, using either the constant provided in 'value' or the corresponding pixel from the buffer on aux as operands. The result is the evaluation of the expression #{formula}\"),
       NULL);
}
#endif
"
    file.close
end
