//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include <maxscript\maxscript.h>
#include <color.h>
#include <maxscript\foundation\colors.h>
#include <maxscript\macros\define_instantiation_functions.h>

def_visible_primitive			( ConvertKelvinToRGB, 		"ConvertKelvinToRGB");

Value* 
ConvertKelvinToRGB_cf(Value **arg_list, int count)
{
	// <color> ConvertColorToRGB <temperature> <intensity>
	// Converts a color temperature in Kelvin and an intensity to a color.
	// Returns the color
	check_arg_count(ConvertKelvinToRGB, 2, count);

	Color col = Color::FromKelvinTemperature(arg_list[0]->to_float(), arg_list[1]->to_float());

	return new ColorValue(col);
}
