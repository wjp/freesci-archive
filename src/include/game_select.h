int game_select_gfxop_init_default(gfx_state_t *state, gfx_options_t *options, void *misc_info);

int game_select_gfxop_init(gfx_state_t *state, int xfact, int yfact, gfx_color_mode_t bpp, gfx_options_t *options, void *misc_info);

int
game_select_display(gfx_driver_t *gfx_driver, const char* freesci_version, char** game_list, int game_count, gfx_bitmap_font_t* font_default, gfx_bitmap_font_t* font_small);

#if 0
/* this can be used to generate code that creates a particular font at runtime */
/* this is meant to be used as a development tool */
void save_font(int id, gfx_bitmap_font_t* font)
#endif 
