/*
#define TIMEOUT_IDS 4
#define TIMEOUT_TIM (uint16_t)(millis() >> 10)
uint8_t timeout_flg = 0;
uint8_t timeout_ovf = 0;
uint16_t timeout_exp[TIMEOUT_IDS];

void timeout_set(uint8_t id, uint16_t val)
{
	uint8_t register msk = (1 << id);
	uint16_t tim = TIMEOUT_TIM;
	val += tim;
	timeout_exp[TIMEOUT_IDS] = val;
	timeout_flg |= msk;
	timeout_ovf |= (tim > exp)?msk:0;
}

uint8_t timeout_exp(uint8_t id)
{
	uint8_t register msk = (1 << id);
	if ((timeout_flg & msk) == 0) return 1;
	uint16_t exp = timeout_exp[TIMEOUT_IDS];
	uint16_t tim = TIMEOUT_TIM;
	if (timeout_ovf & msk)?(tim < exp):(tim > exp)) timeout_flg ^= msk;
	return (timeout_flags & msk)?0:1;
}
*/