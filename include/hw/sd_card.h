#pragma once
#ifdef ENABLE_SD_CARD

#ifndef BOARD_VER_V3
  #error "ENABLE_SD_CARD requires BOARD_VER_V3 — SD_ pins are only defined for that board revision."
#endif

bool setup_sd_card();
bool sd_card_available();

#ifdef ENABLE_SD_CARD_DEBUG
void sd_list_dir(const char* path, uint8_t depth);
#endif

#endif // ENABLE_SD_CARD
