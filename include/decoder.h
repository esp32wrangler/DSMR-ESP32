
#include <map>
#include <deque>
#include <WString.h>

class TFT_eSPI;

typedef std::deque<std::pair<const char*, String>> TMeterValues;

bool decode_telegram(const char * telegram, size_t len, TMeterValues& metervalues);
void update_screen(TMeterValues& metervalues, TFT_eSPI& tft);