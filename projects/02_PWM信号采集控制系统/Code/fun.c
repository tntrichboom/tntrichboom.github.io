#include "fun.h"

void led_show(uint8_t led, uint8_t mode)
{
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
	
	if(mode)
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8<<(led-1), GPIO_PIN_RESET);
	else
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8<<(led-1), GPIO_PIN_SET);
	
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
}
//
double adc_R37, adc_R38;
uint16_t fre_R40;
//
uint8_t lcd_mode;
// 监控界面
uint16_t PWM_CF;
uint8_t PWM_CD;
uint8_t ST_flag;
uint8_t time_clock[3];
// 统计界面
uint8_t RECD_flag;
//
uint16_t PWM_CF_no;
uint8_t PWM_CD_no;
uint16_t fre_R40_no;
uint8_t time_clock_no[3];
uint16_t RECD_XF;
// 参数界面
int32_t PARA_DFSR[4] = {1, 80, 100, 2000}; // 显示值
int32_t PARA_DFSR_past[4] = {1, 80, 100, 2000}; // 换算
int32_t PARA_DFSR_last[4] = {1, 80, 100, 2000}; // 上一次的值
uint8_t DFSR_indax;
//
uint8_t led_flag;
uint8_t B1_data,B2_data,B3_data,B4_data,B1_last,B2_last = 1,B3_last,B4_last;
uint32_t time_dece;
uint32_t time_2s;
void key_scan(void)
{
	if(uwTick-time_dece < 10) return;
	time_dece = uwTick;
	B1_data = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);B2_data = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
	B3_data = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);B4_data = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
	
	if(!B1_data&B1_last) // B1
	{
		if(lcd_mode == 1) // 保存上一次的值
		{
			for(int i=0;i<4;i++) PARA_DFSR_last[i] = PARA_DFSR_past[i];
		}
		if(++lcd_mode > 2) //2 -> 0
		{
			lcd_mode = 0;
			DFSR_indax = 0;
			if(PARA_DFSR[0]>(PARA_DFSR[1]-10)||PARA_DFSR[2]>(PARA_DFSR[3]-1000) 
			|| PARA_DFSR[1]<0 || PARA_DFSR[1] > 100 || PARA_DFSR[3] < 0) // 非法状态
			{
				for(int i=0;i<4;i++)
				{
					PARA_DFSR_past[i] = PARA_DFSR_last[i]; 
					PARA_DFSR[i] = PARA_DFSR_last[i]; 
				}
			}
			else // 合法
			{
				for(int i=0;i<4;i++) PARA_DFSR_past[i] = PARA_DFSR[i];
			}
		}
	}
	
	if(lcd_mode == 2) // 参数界面
	{
		if(!B2_data&B2_last) // B2
		{
			if(++DFSR_indax > 3) DFSR_indax = 0; // 索引值
		}
		if(!B3_data&B3_last) // B3  +
		{
			PARA_DFSR[DFSR_indax]+=pow(10,DFSR_indax);
		}
		if(!B4_data&B4_last) // B4  - 
		{
			PARA_DFSR[DFSR_indax]-=pow(10,DFSR_indax);
		}
	}
	else if(lcd_mode == 0) // 监控界面
	{
		if(!B2_data&B2_last)
		{
			time_2s = uwTick;
		}
		if(B2_data&!B2_last)
		{
			if(uwTick-time_2s>=2000) // 清零
			{
				memset(time_clock, 0, 3);
			}
			else // 
			{
				ST_flag ^= 1;
			}
		}
	}
	
	B1_last = B1_data;B2_last = B2_data;B3_last = B3_data;B4_last = B4_data;
}
//
uint32_t lcd_dece;
char text[20];
void lcd_show(void)
{
	if(uwTick-lcd_dece<99) return;
	lcd_dece = uwTick;
	uint16_t temp = GPIOC -> ODR;
	
	if(lcd_mode == 0)
	{
		sprintf(text, "       PWM          ");
		LCD_DisplayStringLine(Line1, (uint8_t *)text);
		sprintf(text, "   CF=%dHz          ", PWM_CF);
		LCD_DisplayStringLine(Line3, (uint8_t *)text);
		sprintf(text, "   CD=%d%%          ", PWM_CD);
		LCD_DisplayStringLine(Line4, (uint8_t *)text);
		sprintf(text, "   DF=%dHz          ", fre_R40);
		LCD_DisplayStringLine(Line5, (uint8_t *)text);
		if(!ST_flag) sprintf(text, "   ST=UNLOCK        ");
		else sprintf(text, "   ST=LOCK          ");
		LCD_DisplayStringLine(Line6, (uint8_t *)text);
		sprintf(text, "   %02dH%02dM%02dS  ", time_clock[0], time_clock[1], time_clock[2]);
		LCD_DisplayStringLine(Line7, (uint8_t *)text);
	}
	else if(lcd_mode == 1)
	{
		sprintf(text, "       RECD         ");
		LCD_DisplayStringLine(Line1, (uint8_t *)text);
		sprintf(text, "   CF=%dHz          ", PWM_CF_no);
		LCD_DisplayStringLine(Line3, (uint8_t *)text);
		sprintf(text, "   CD=%d%%          ", PWM_CD_no);
		LCD_DisplayStringLine(Line4, (uint8_t *)text);
		sprintf(text, "   DF=%dHz          ", fre_R40_no);
		LCD_DisplayStringLine(Line5, (uint8_t *)text);
		sprintf(text, "   XF=%dHz          ", RECD_XF);
		LCD_DisplayStringLine(Line6, (uint8_t *)text);
		sprintf(text, "   %02dH%02dM%02dS  ", time_clock_no[0], time_clock_no[1], time_clock_no[2]);
		LCD_DisplayStringLine(Line7, (uint8_t *)text);
	}
	else
	{
		sprintf(text, "       PARA         ");
		LCD_DisplayStringLine(Line1, (uint8_t *)text);
		sprintf(text, "   DS=%d%%          ", PARA_DFSR[0]);
		LCD_DisplayStringLine(Line3, (uint8_t *)text);
		sprintf(text, "   DR=%d%%          ", PARA_DFSR[1]);
		LCD_DisplayStringLine(Line4, (uint8_t *)text);
		sprintf(text, "   FS=%dHz          ", PARA_DFSR[2]);
		LCD_DisplayStringLine(Line5, (uint8_t *)text);
		sprintf(text, "   FR=%dHz          ", PARA_DFSR[3]);
		LCD_DisplayStringLine(Line6, (uint8_t *)text);
		LCD_ClearLine(Line7);
	}
	
	GPIOC -> ODR = temp;
}
//
double adc_read(ADC_HandleTypeDef *hadc)
{
	HAL_ADC_Start(hadc);
	uint16_t value_adc = HAL_ADC_GetValue(hadc);
	return 3.3*value_adc/4095;
}
void data_proc(void)
{
	adc_R37 = adc_read(&hadc2);
	adc_R38 = adc_read(&hadc1);
	if(!ST_flag)
	{
		// 阶梯计算 占空比
		uint16_t count_N = (PARA_DFSR_past[1]-10) / PARA_DFSR_past[0];
		double VA = 3.3/(count_N+1);
		for(int i=0;i<count_N+1;i++)
		{
			if(adc_R37 >= i*VA && adc_R37 < (i+1)*VA)
			{
				PWM_CD = 10+PARA_DFSR_past[0]*i;
				TIM17->CCR1 = PWM_CD;
				break;
			}
		}
		//  阶梯计算 占空比
		uint16_t count_N_f = (PARA_DFSR_past[3]-1000) / PARA_DFSR_past[2];
		double VA_f = 3.3/(count_N_f+1);
		for(int j=0;j<count_N_f+1;j++)
		{
			if(adc_R38 >= j*VA_f && adc_R38 < (j+1)*VA_f)
			{
				PWM_CF = 1000+PARA_DFSR_past[2]*j;
				TIM17->PSC = 800000/PWM_CF - 1; 
				break;
			}
		}
	}
	// 异常状态
	if(!RECD_flag)
	{
		if(abs(PWM_CF - fre_R40) > 1000)
		{
			RECD_flag = 1; // 异常
			RECD_XF = abs(PWM_CF - fre_R40);
			PWM_CF_no = PWM_CF;
			PWM_CD_no = PWM_CD;
			fre_R40_no = fre_R40;
			for(int i=0;i<3;i++)
			{
				time_clock_no[i] = time_clock[i];
			}
		}
	}
	else
	{
		if(abs(PWM_CF - fre_R40) <= 1000)
		{
			RECD_flag = 0; // 无异常
		}
	}
}
void main_pros(void)
{
	led_show(1, (lcd_mode == 0));
	led_show(2, ST_flag);
	led_show(3, RECD_flag);
	lcd_show();
	key_scan();
	data_proc();
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if(htim -> Instance == TIM2)
	{
		fre_R40 = 1000000/(TIM2 -> CCR1 + 1);
		TIM2 -> CNT = 0;
	}
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim -> Instance == TIM3)
	{
		if(++time_clock[2] > 59)
		{
			time_clock[2] = 0;
			if(++time_clock[1] > 59)
			{
				time_clock[1] = 0;
				if(++time_clock[0] > 99)
				{
					time_clock[0] = 0;
					time_clock[1] = 0;
					time_clock[2] = 0;
				}
			}
		}
	}
}
