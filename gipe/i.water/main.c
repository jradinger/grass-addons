/****************************************************************************
 *
 * MODULE:       i.water
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates if water is there (value=1)
 * 		 two versions, 1) generic (albedo,ndvi)
 * 		 2) Modis (surf_refl_7,ndvi)
 *
 * COPYRIGHT:    (C) 2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


double water(double albedo, double ndvi);
double water_modis(double surf_ref_7, double ndvi);

int main(int argc, char *argv[])
{
	struct Cell_head cellhd; //region+header info
	char *mapset; // mapset name
	int nrows, ncols;
	int row,col;

	struct GModule *module;
	struct Option *input1, *input2, *input3, *output1;
	
	struct Flag *flag1;	
	struct History history; //metadata
	
	/************************************/
	/* FMEO Declarations*****************/
	char *name;   // input raster name
	char *result1; //output raster name
	//File Descriptors
	int infd_ndvi, infd_albedo, infd_ref7;
	int outfd1;
	
	char *ndvi,*albedo,*ref7;
	int i=0,j=0;
	
	void *inrast_ndvi, *inrast_albedo, *inrast_ref7;
	CELL *outrast1;
	RASTER_MAP_TYPE data_type_output=CELL_TYPE;
	RASTER_MAP_TYPE data_type_ndvi;
	RASTER_MAP_TYPE data_type_albedo;
	RASTER_MAP_TYPE data_type_ref7;
	/************************************/
	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("water, detection");
	module->description = _("Water detection, 1 if found, 0 if not");

	/* Define the different options */
	input1 = G_define_standard_option(G_OPT_R_INPUT) ;
	input1->key	   = _("ndvi");
	input1->description=_("Name of the NDVI map [-]");
	input1->answer     =_("ndvi");

	input2 = G_define_standard_option(G_OPT_R_INPUT) ;
	input2->key        =_("albedo");
	input2->required   = NO ;
	input2->description=_("Name of the Albedo map [-]");

	input3 = G_define_standard_option(G_OPT_R_INPUT) ;
	input3->key        =_("Modref7");
	input3->required   = NO ;
	input3->description=_("Name of the Modis surface reflectance band 7 [-]");

	output1 = G_define_standard_option(G_OPT_R_OUTPUT) ;
	output1->key        =_("water");
	output1->description=_("Name of the output water layer [0/1]");
	output1->answer     =_("water");
	/********************/
	if (G_parser(argc, argv))
		exit (EXIT_FAILURE);

	ndvi	 	= input1->answer;
	if(input2->answer)
	albedo	 	= input2->answer;
	if(input3->answer)
	ref7		= input3->answer;
	

	if(!input2->answer&&!input3->answer){
		G_fatal_error(_("ERROR: needs either Albedo or Modis surface reflectance in band 7, bailing out."));
	}
	
	result1  = output1->answer;
	/***************************************************/
	mapset = G_find_cell2(ndvi, "");
	if (mapset == NULL) {
		G_fatal_error(_("cell file [%s] not found"), ndvi);
	}
	data_type_ndvi = G_raster_map_type(ndvi,mapset);
	if ( (infd_ndvi = G_open_cell_old (ndvi,mapset)) < 0)
		G_fatal_error (_("Cannot open cell file [%s]"), ndvi);
	if (G_get_cellhd (ndvi, mapset, &cellhd) < 0)
		G_fatal_error (_("Cannot read file header of [%s])"), ndvi);
	inrast_ndvi = G_allocate_raster_buf(data_type_ndvi);
	/***************************************************/
	if(input2->answer){
	mapset = G_find_cell2 (albedo, "");
	if (mapset == NULL) {
		G_fatal_error(_("cell file [%s] not found"),albedo);
	}
	data_type_albedo = G_raster_map_type(albedo,mapset);
	if ( (infd_albedo = G_open_cell_old (albedo,mapset)) < 0)
		G_fatal_error(_("Cannot open cell file [%s]"), albedo);
	if (G_get_cellhd (albedo, mapset, &cellhd) < 0)
		G_fatal_error(_("Cannot read file header of [%s]"), albedo);
	inrast_albedo = G_allocate_raster_buf(data_type_albedo);
	}
	/***************************************************/
	if(input3->answer){
	mapset = G_find_cell2 (ref7, "");
	if (mapset == NULL) {
		G_fatal_error(_("Cell file [%s] not found"), ref7);
	}
	data_type_ref7 = G_raster_map_type(ref7,mapset);
	if ( (infd_ref7 = G_open_cell_old (ref7,mapset)) < 0)
		G_fatal_error(_("Cannot open cell file [%s]"), ref7);
	if (G_get_cellhd (ref7, mapset, &cellhd) < 0)
		G_fatal_error(_("Cannot read file header of [%s]"), ref7);
	inrast_ref7 = G_allocate_raster_buf(data_type_ref7);
	}
	/***************************************************/
	G_debug(3, "number of rows %d",cellhd.rows);
	nrows = G_window_rows();
	ncols = G_window_cols();
	outrast1 = G_allocate_raster_buf(data_type_output);
	/* Create New raster files */
	if ( (outfd1 = G_open_raster_new (result1,data_type_output)) < 0)
		G_fatal_error(_("Could not open <%s>"),result1);
	/* Process pixels */
	for (row = 0; row < nrows; row++)
	{
		CELL d;
		DCELL d_ndvi;
		DCELL d_albedo;
		DCELL d_ref7;
		G_percent(row,nrows,2);
		/* read input maps */	
		if(G_get_raster_row(infd_ndvi,inrast_ndvi,row,data_type_ndvi)<0)
			G_fatal_error(_("Could not read from <%s>"),ndvi);
		if(input2->answer){
		if(G_get_raster_row(infd_albedo,inrast_albedo,row,data_type_albedo)<0)
			G_fatal_error(_("Could not read from <%s>"),albedo);
		}
		if(input3->answer){
		if(G_get_raster_row(infd_ref7,inrast_ref7,row,data_type_ref7)<0)
			G_fatal_error(_("Could not read from <%s>"),ref7);
		}
		/*process the data */
		for (col=0; col < ncols; col++)
		{
			switch(data_type_ndvi){
				case CELL_TYPE:
					d_ndvi = (double) ((CELL *) inrast_ndvi)[col];
					break;
				case FCELL_TYPE:
					d_ndvi = (double) ((FCELL *) inrast_ndvi)[col];
					break;
				case DCELL_TYPE:
					d_ndvi = ((DCELL *) inrast_ndvi)[col];
					break;
			}
			if(input2->answer){
			switch(data_type_albedo){
				case CELL_TYPE:
					d_albedo = (double) ((CELL *) inrast_albedo)[col];
					break;
				case FCELL_TYPE:
					d_albedo = (double) ((FCELL *) inrast_albedo)[col];
					break;
				case DCELL_TYPE:
					d_albedo = ((DCELL *) inrast_albedo)[col];
					break;
			}
			}
			if(input3->answer){
			switch(data_type_ref7){
				case CELL_TYPE:
					d_ref7 = (double) ((CELL *) inrast_ref7)[col];
					break;
				case FCELL_TYPE:
					d_ref7 = (double) ((FCELL *) inrast_ref7)[col];
					break;
				case DCELL_TYPE:
					d_ref7 = ((DCELL *) inrast_ref7)[col];
					break;
			}
			}
			if(G_is_d_null_value(&d_ndvi)||
			(input2->answer&&G_is_d_null_value(&d_albedo))||
			(input3->answer&&G_is_d_null_value(&d_ref7))){
				G_set_c_null_value(&outrast1[col],1);
			}else {
				/************************************/
				/* calculate water detection	    */
				if(input2->answer){
					d = water(d_albedo,d_ndvi);
				} else if (input3->answer){
					d = water_modis(d_ref7,d_ndvi);
				}
				outrast1[col] = d;
			}
		}
		if (G_put_raster_row (outfd1, outrast1, data_type_output) < 0)
			G_fatal_error(_("Cannot write to output raster file"));
	}

	G_free (inrast_ndvi);
	G_close_cell (infd_ndvi);
	if(input2->answer){
		G_free (inrast_albedo);
		G_close_cell (infd_albedo);
	}
	if(input3->answer){
		G_free (inrast_ref7);
		G_close_cell (infd_ref7);
	}
	G_free (outrast1);
	G_close_cell (outfd1);

	G_short_history(result1, "raster", &history);
	G_command_history(&history);
	G_write_history(result1,&history);

	exit(EXIT_SUCCESS);
}

