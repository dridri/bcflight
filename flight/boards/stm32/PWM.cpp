/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/


#include <stdint.h>
#include "PWM.h"
#include "GPIO.h"
#include "Debug.h"

uint8_t PWM::mTimersInitialized = 0b00000000;

#define A 0
#define B 1
#define C 2
#define D 3
#define E 4
#define F 5
#define PIN( port, pin ) ( ( port << 4 ) | ( pin & 0x0F ) )

typedef struct {
	uint8_t pin; // 0xPN, P=Port, N=pin number
	TIM_TypeDef* tim;
	uint8_t channel;
} TimerMap;


#ifdef VARIANT_GROUP_stm32f4
static const TimerMap timer_map[] = {
	{ PIN( A, 8 ), TIM1, TIM_CHANNEL_1 },
	{ PIN( A, 9 ), TIM1, TIM_CHANNEL_2 },
	{ PIN( A, 10 ), TIM1, TIM_CHANNEL_3 },
	{ PIN( A, 11 ), TIM1, TIM_CHANNEL_4 },
	{ PIN( A, 0 ), TIM2, TIM_CHANNEL_1 },
	{ PIN( A, 1 ), TIM2, TIM_CHANNEL_2 },
	{ PIN( A, 2 ), TIM2, TIM_CHANNEL_3 },
	{ PIN( A, 3 ), TIM2, TIM_CHANNEL_4 },
	{ PIN( C, 6 ), TIM3, TIM_CHANNEL_1 },
	{ PIN( C, 7 ), TIM3, TIM_CHANNEL_2 },
	{ PIN( C, 8 ), TIM3, TIM_CHANNEL_3 },
	{ PIN( C, 9 ), TIM3, TIM_CHANNEL_4 },
	{ PIN( D, 12 ), TIM4, TIM_CHANNEL_1 },
	{ PIN( D, 13 ), TIM4, TIM_CHANNEL_2 },
	{ PIN( D, 14 ), TIM4, TIM_CHANNEL_3 },
	{ PIN( D, 15 ), TIM4, TIM_CHANNEL_4 },
};
#endif


PWM::PWM( uint32_t pin, uint32_t time_base, uint32_t period_time, uint32_t sample_time, PWMMode mode, bool loop )
{
	uint32_t i = 0;
	for ( i = 0; i < sizeof(timer_map)/sizeof(TimerMap); i++ ) {
		if ( timer_map[i].pin == pin ) {
			mHandle.Instance = timer_map[i].tim;
			mChannel = timer_map[i].channel;
			break;
		}
	}

	if ( ( mTimersInitialized & ( 1 << i ) ) == 0 ) {
		mTimersInitialized |= ( 1 << i );
		uint32_t gpio_af = 0;
		uint8_t resolution = 16;
		uint32_t core_clock = HAL_RCC_GetPCLK1Freq();

		if ( mHandle.Instance == TIM1 ) {
			__HAL_RCC_TIM1_CLK_ENABLE();
			gpio_af = GPIO_AF1_TIM1;
			core_clock = HAL_RCC_GetHCLKFreq();
		} else if ( mHandle.Instance == TIM2 ) {
			__HAL_RCC_TIM2_CLK_ENABLE();
			gpio_af = GPIO_AF1_TIM2;
			resolution = 32;
		} else if ( mHandle.Instance == TIM3 ) {
			__HAL_RCC_TIM3_CLK_ENABLE();
			gpio_af = GPIO_AF2_TIM3;
		} else if ( mHandle.Instance == TIM4 ) {
			__HAL_RCC_TIM4_CLK_ENABLE();
			gpio_af = GPIO_AF2_TIM4;
		} else if ( mHandle.Instance == TIM5 ) {
			__HAL_RCC_TIM5_CLK_ENABLE();
			gpio_af = GPIO_AF2_TIM5;
			resolution = 32;
		}

		GPIO_TypeDef* b = GPIO::_base( pin );
		if ( b == nullptr ) {
			return;
		}
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_InitStruct.Pin = 1 << ( pin & 0x0F );
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Alternate = gpio_af;
		HAL_GPIO_Init( b, &GPIO_InitStruct );

		uint32_t freq = time_base / period_time;
		uint32_t period = core_clock / freq;
		gDebug() << "full period : " << period << "\n";
		uint32_t prescaler = ( period / 0xFFFF ) * 2 + 1;
		gDebug() << "prescaler : " << prescaler << "\n";
		period = period % 0xFFFF;
		gDebug() << "period : " << period << "\n";
		gDebug() << "half : " << period/2 << "\n";

		mPeriodUs = (uint32_t)( (uint64_t)period * (uint64_t)time_base / (uint64_t)core_clock );

		mHandle.Init.Prescaler = prescaler;
		mHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
		mHandle.Init.Period = period - 1;
		mHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
		HAL_TIM_PWM_Init( &mHandle );

		TIM_MasterConfigTypeDef sMasterConfig;

		sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
		sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
		HAL_TIMEx_MasterConfigSynchronization( &mHandle, &sMasterConfig );
	}

		TIM_OC_InitTypeDef sConfigOC;
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_SET;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
	HAL_TIM_PWM_ConfigChannel( &mHandle, &sConfigOC, mChannel );

	__HAL_TIM_SET_COMPARE( &mHandle, mChannel, 0 );

	HAL_TIM_PWM_Start( &mHandle, mChannel );

	__HAL_TIM_SET_COMPARE( &mHandle, mChannel, 0 );

}


PWM::~PWM()
{
}


void PWM::SetPWMDuty( uint16_t width_16bits )
{
	__HAL_TIM_SET_COMPARE( &mHandle, mChannel, (uint64_t)width_16bits * (uint64_t)mHandle.Init.Period / 65535ULL );
}


void PWM::SetPWMus( uint32_t width_us )
{
	__HAL_TIM_SET_COMPARE( &mHandle, mChannel, (uint64_t)width_us * (uint64_t)mHandle.Init.Period / (uint64_t)mPeriodUs );
}


void PWM::SetPWMBuffer( uint8_t* buffer, uint32_t len )
{
}


void PWM::Update()
{
}
