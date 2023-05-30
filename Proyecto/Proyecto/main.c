#include "def.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "common_func.h"
#include "common_types.h"
#include "lcd.h"


const char keypadLayout[4][4] = {{'1', '2', '3', 'A'},
								 {'4', '5', '6', 'B'},
								 {'7', '8', '9', 'C'},
								 {'*', '0', '#', 'D'}};
							// 1	2    4    8    10    20
const double noteLengths[6] = {4.0, 2.0, 1.0, 0.5, 0.25, 0.125};
const u8 noteDefinitions[61][2] = {{0, 0}, {118,64},{112,64},{105,64},{100,64},{94,64},{89,64},{84,64},{79,64},{74,64},{70,64},{66,64},{63,64},{59,64},{55,64},{52,64},{50,64},{46,64},{44,64},{41,64},{39,64},{37,64},{35,64},{32,64},{252,8},{238,8},{224,8},{212,8},{200,8},{189,8},{178,8},{168,8},{158,8},{149,8},{141,8},{133,8},{126,8},{118,8},{112,8},{105,8},{99,8},{94,8},{89,8},{84,8},{79,8},{74,8},{70,8},{66,8},{62,8},{59,8},{55,8},{52,8},{49,8},{46,8},{44,8},{41,8},{39,8},{37,8},{35,8},{33,8},{31,8}};
const u8 a4Index = 34;
const u8 noteDefSize = 61;

const u8 notPlaying = 0;
const u8 playingMelody = 1;
const u8 playSingleNote = 2;

u8 beats_per_minute = 116;
double timer_length = 60.0 / (double)(116); //Length of a beat, in seconds
double timer_counter = 0.0; //In ms
u8 melody_index = 0;
u8 is_playing = 0;

const u8 melodyLimit = 200;
u8 melody[201][2] = {{0,	0x00}};

void SetUpRegisters();
char ReadFromKeypad();

void PlayMelody();
double GetNoteLength(const u8 length_bitmask);

void MelodyEditor();
void GenerateEditorView();
u8 IsNoteEOF(u8 note_idx);
void InsertNote(u8 melody_idx);
void ReplaceNote(u8 melody_idx);
void DeleteNote(u8 melody_idx);
void SetBPM();

u8 SelectNote();
u8 SelectLength();
void PlaySingleNote(u8 note_idx);
void PlayNote(u8 note_idx);
void PrintNote(u8 note_idx);
void PrintNoteLength(u8 note_length);

void SaveToEEPROM();
void LoadFromEEPROM();
void CleanMelody();

int main() {
	
	SetUpRegisters();
	
	while (1) {
		MelodyEditor();
		//PlayMelody();
		//_delay_ms(1000);
		//SelectNote();
		//SelectLength();
	}
}

ISR(TIMER2_COMP_vect) {
	if (is_playing == notPlaying) {
		return;
	}
	
	if (is_playing == playingMelody){
		if (timer_counter > 0.0) {
			timer_counter -= 1.0;
			if (timer_counter <= 0.0) {
				TCCR0 &= ~_BV(COM00);
				melody_index++;
			}
		} else {
			PlayNote(melody[melody_index][0]);
			timer_counter = GetNoteLength(melody[melody_index][1]) * timer_length * 1000.0;
		}
	}
	
	if (is_playing == playSingleNote) {
		if (timer_counter > 0.0) {
			timer_counter -= 1.0;
			if (timer_counter <= 0.0) {
				TCCR0 &= ~_BV(COM00);
				is_playing = notPlaying;
			}
		}
	}
}

void SetUpRegisters() {
	//PORTA: LCD
	//PORTB: TIMER0
	//PORTC: KEYPAD
	lcd_init(LCD_DISP_ON_CURSOR_BLINK);
	
	DDRB = 0x08;
	PORTB = 0x00;
	
	DDRC = 0xF0;
	PORTC = 0xFF;
	TCNT0 = 0;
	TCCR0 |= _BV(WGM01);
	
	TIFR |= _BV(OCF2);
	TIMSK |= _BV(OCIE2);
	TCNT2 = 0;
	OCR2 = 124;
	TCCR2 |= _BV(WGM21) | _BV(CS21);
	
	sei();
	return;
}

char ReadFromKeypad() {
	for (u8 i = 4; i < 8; i++) {
		ClearBit(&PORTC, i);
		_delay_ms(btn_delay_ms);
		for (u8 e = 0; e < 4; e++) {
			if(DelayIfClear(&PINC, e)) {
				SetBit(&PORTC, i);
				return keypadLayout[7 - i][3 - e];
			}
		}
		SetBit(&PORTC, i);
		_delay_ms(btn_delay_ms);
	}
	return '\0';
}

void PlayMelody() {
	//melody_index = 0;
	timer_counter = 0.0;
	is_playing = 1;
	while (!IsNoteEOF(melody_index) && melody_index < melodyLimit) {
		char response = ReadFromKeypad();
		if (response == 'A') {
			break;
		}
		GenerateEditorView();
	}
	is_playing = 0;
	TCCR0 &= ~_BV(COM00);
	return;
}

double GetNoteLength(const u8 length_bitmask) {
	double total_length = 0.0;
	for (u8 i = 0; i < 6; i++) {
		if ((length_bitmask & (0x01 << i)) != 0) {
			total_length += noteLengths[i];
		}
	}
	return total_length;
}

void MelodyEditor() {
	melody_index = 0;
	u8 regenerate_view = 1;
	u8 play_note = 1;
	u8 old_value;
	while(1) {
		if (regenerate_view != 0){
			GenerateEditorView();
			if (play_note != 0) {
				PlaySingleNote(melody[melody_index][0]);
			} else {
				play_note = 1;
			}
			regenerate_view = 0;
		}
		
		char resp = ReadFromKeypad();
		switch(resp) {
		case '5':
			PlaySingleNote(melody[melody_index][0]);
			break;
		case '4':
			if (melody_index > 0) {
				melody_index--;
			}
			regenerate_view = 1;
			break;
		case '6':
			if (melody[melody_index + 1][1] != 0x00) {
				melody_index++;
			}
			regenerate_view = 1;
			break;
		case '2':
			if (melody_index + 10 < melodyLimit) {
				melody_index += 10;
			} else {
				melody_index = melodyLimit - 1;
			}
			while (melody[melody_index][1] == 0x00) {
				melody_index--;
			}
			regenerate_view = 1;
			break;
		case '8':
			if (melody_index >= 12) {
				melody_index -= 12;
			} else {
				melody_index = 0;
			}
			regenerate_view = 1;
			break;
		case 'A':
			InsertNote(melody_index);
			regenerate_view = 1;
			break;
		case 'B':
			ReplaceNote(melody_index);
			regenerate_view = 1;
			break;
		case 'D':
			DeleteNote(melody_index);
			regenerate_view = 1;
			break;
		case 'C':
			SetBPM();
			regenerate_view = 1;
			break;
		case '*':
			SaveToEEPROM();
			regenerate_view = 1;
			break;
		case '0':
			old_value = melody_index;
			PlayMelody();
			melody_index = old_value;
			regenerate_view = 1;
			play_note = 0;
			break;
		case '#':
			LoadFromEEPROM();
			regenerate_view = 1;
			break;
		default:
			break;
		}
	}
}

void GenerateEditorView() {
	lcd_clrscr();
	lcd_command(LCD_DISP_ON_CURSOR_BLINK);
	if (melody_index > 0) {
		lcd_gotoxy(0, 0);
		PrintNote(melody[melody_index - 1][0]);
		lcd_gotoxy(0, 1);
		PrintNoteLength(melody[melody_index - 1][1]);
	}
	lcd_gotoxy(4, 0);
	PrintNote(melody[melody_index][0]);
	lcd_gotoxy(4, 1);
	PrintNoteLength(melody[melody_index][1]);
	if (melody[melody_index + 1][1] != 0x00) {
		lcd_gotoxy(8, 0);
		PrintNote(melody[melody_index + 1][0]);
		lcd_gotoxy(8, 1);
		PrintNoteLength(melody[melody_index + 1][1]);	
		if (melody[melody_index + 2][1] != 0x00) {
			lcd_gotoxy(12, 0);
			PrintNote(melody[melody_index + 2][0]);
			lcd_gotoxy(12, 1);
			PrintNoteLength(melody[melody_index + 2][1]);
		}
	}
	lcd_gotoxy(4, 0);
}

u8 IsNoteEOF(u8 note_idx) {
	return melody[melody_index][1] == 0;
}

void InsertNote(u8 melody_idx) {
	u8 note = SelectNote();
	if (note == 255) {
		return;
	}
	u8 length = SelectLength();
	if (length == 255) {
		return;
	}
	u8 tmp;
	do {
		tmp = melody[melody_idx][0];
		melody[melody_idx][0] = note;
		note = tmp;
		tmp = melody[melody_idx][1];
		melody[melody_idx][1] = length;
		length = tmp;
	} while (melody[melody_idx++][1] != 0x00);
	melody[melodyLimit - 1][0] = 0x00;
	melody[melodyLimit - 1][1] = 0x00;
}

void ReplaceNote(u8 melody_idx) {
	if (melody[melody_idx][1] == 0) {
		InsertNote(melody_idx);
		return;
	}
	u8 note = SelectNote();
	if (note == 255) {
		return;
	}
	u8 length = SelectLength();
	if (length == 255) {
		return;
	}
	melody[melody_idx][0] = note;
	melody[melody_idx][1] = length;
}

void DeleteNote(u8 melody_idx) {
	while (melody[melody_idx][1] != 0x00) {
		melody[melody_idx][0] = melody[melody_idx + 1][0];
		melody[melody_idx][1] = melody[melody_idx + 1][1];
		melody_idx++;
	}
	melody[melody_idx][0] = 0x00;
	melody[melody_idx][1] = 0x00;
}

void SetBPM() {
	lcd_clrscr();
	lcd_command(LCD_DISP_ON);
	lcd_gotoxy(0, 0);
	lcd_puts("BPM: 0");
	u16 bpm = 0;
	char bpm_str[5] = {};
	u8 resp;
	while (1) {
		resp = ReadFromKeypad();
		if (resp == 'A') {
			if (bpm == 0) {
				bpm = 60;
			}
			beats_per_minute = bpm;
			timer_length = 60.0 / (double)beats_per_minute;
			return;
		} else if (resp == 'B') {
			return;
		} else if (resp == 'D') {
			bpm /= 10;
		} else if (resp < '0' || resp > '9') {
			continue;
		} else {
			bpm *= 10;
			bpm += resp - '0';
			if (bpm > 255) {
				bpm = 255;
				lcd_puts("255");
				continue;
			}
		}
		lcd_gotoxy(5, 0);
		lcd_puts("   ");
		lcd_gotoxy(5, 0);
		sprintf(bpm_str, "%d", bpm);
		lcd_puts(bpm_str);
	}
}

u8 SelectNote() {
	u8 idx = a4Index;
	u8 play_note = 1;
	
	lcd_clrscr();
	lcd_gotoxy(5, 0);
	lcd_puts("A: Select");
	lcd_gotoxy(5, 1);
	lcd_puts("B: Cancel");
	lcd_command(LCD_DISP_ON);
	while (1) {
		if (play_note) {
			PlaySingleNote(idx);
			play_note = 0;
		}
		
		lcd_gotoxy(0, 0);
		PrintNote(idx);
		
		char resp = ReadFromKeypad();
		switch (resp) {
		case 'A':
			return idx;
			break;
		case 'B':
			return 255;
			break;
		case '0':
			return 0;
			break;
		case '5':
			play_note = 1;
			break;
		case '4':
			if (idx > 1) {
				idx--;
			}
			play_note = 1;
			break;
		case '6':
			if (idx + 1 < noteDefSize) {
				idx++;
			}
			play_note = 1;
			break;
		case '2':
			if (idx + 12 < noteDefSize) {
				idx += 12;
			} else {
				idx = noteDefSize - 1;
			}
			play_note = 1;
			break;
		case '8':
			if (idx > 12) {
				idx -= 12;
			} else {
				idx = 1;
			}
			play_note = 1;
			break;
		default:
			break;
		}
		
		//_delay_ms(1);
	}
}

u8 SelectLength() {
	lcd_clrscr();
	lcd_command(LCD_DISP_ON);
	lcd_gotoxy(0, 1);
	lcd_puts("w h q o s t");
	
	u8 length = 0;
	
	while (1) {
		for (u8 i = 0; i < 6; i++) {
			lcd_gotoxy(i * 2, 0);
			if ((length & (1 << i)) == 0) {
				lcd_putc('0');
			} else {
				lcd_putc('1');
			}
		}
		char resp = ReadFromKeypad();
		
		if (resp >= '0' && resp <= '5') {
			length ^= 1 << (resp - '0');
		}
		switch(resp) {
		case 'A':
			if (length == 0x00) {
				return 0x04;
			}
			return length;
			break;
		case 'B':
			return 255;
			break;
		default:
			break;
		}
	}
}

void PlaySingleNote(u8 note_idx) {
	PlayNote(note_idx);
	timer_counter = 1000.0;
	is_playing = playSingleNote;
}

void PlayNote(u8 note_idx) {
	if (note_idx == 0) {
		TCCR0 &= ~_BV(COM00);
	} else {
		TCCR0 |= _BV(COM00);
		TCCR0 &= ~0x07;
		if (noteDefinitions[note_idx][1] == 8) {
			TCCR0 |= _BV(CS01);
		} else {
			TCCR0 |= _BV(CS01) | _BV(CS00);
		}
		OCR0 = noteDefinitions[note_idx][0];
	}
}

void PrintNote(u8 note_idx) {
	if (note_idx == 0) {
		lcd_puts("---");
		return;
	}
	switch (note_idx % 12) {
	case 0:
		lcd_puts("B ");
		break;
	case 1:
		lcd_puts("C ");
		break;
	case 2:
		lcd_puts("C#");
		break;
	case 3:
		lcd_puts("D ");
		break;
	case 4:
		lcd_puts("D#");
		break;
	case 5:
		lcd_puts("E ");
		break;
	case 6:
		lcd_puts("F ");
		break;
	case 7:
		lcd_puts("F#");
		break;
	case 8:
		lcd_puts("G ");
		break;
	case 9:
		lcd_puts("G#");
		break;
	case 10:
		lcd_puts("A ");
		break;
	case 11:
		lcd_puts("A#");
		break;
	}
	lcd_putc('2' + ((note_idx - 1) / 12));
}

void PrintNoteLength(u8 note_length) {
	u8 nibble = note_length >> 4;
	if (nibble > 9) {
		lcd_putc('A' + nibble - 10);
	} else {
		lcd_putc('0' + nibble);
	}
	nibble = note_length & 0x0F;
	if (nibble > 9) {
		lcd_putc('A' + nibble - 10);
	} else {
		lcd_putc('0' + nibble);
	}
	return;
}

void SaveToEEPROM() {
	lcd_clrscr();
	lcd_command(LCD_DISP_ON);
	lcd_puts("A: Save");
	lcd_gotoxy(0, 1);
	lcd_puts("B: Cancel");
	while (1) {
		u8 resp = ReadFromKeypad();
		if (resp == 'A') {
			break;
		} else if (resp == 'B') {
			return;
		}
	}
	CleanMelody();
	eeprom_write_word((u16*)0, beats_per_minute);
	for (u16 i = 1; i < melodyLimit + 2; i++) {
		u16 temp = ((u16)(melody[i - 1][0]) << 8) | (u16)melody[i - 1][1];
		eeprom_write_word((u16*)(i * 2), temp);
	}
}

void LoadFromEEPROM() {
	lcd_clrscr();
	lcd_command(LCD_DISP_ON);
	lcd_puts("A: Load");
	lcd_gotoxy(0, 1);
	lcd_puts("B: Cancel");
	while (1) {
		u8 resp = ReadFromKeypad();
		if (resp == 'A') {
			break;
		} else if (resp == 'B') {
			return;
		}
	}
	beats_per_minute = eeprom_read_word((u16*)0);
	timer_length = 60.0 / (double)beats_per_minute;
	for (u16 i = 1; i < melodyLimit + 2; i++) {
		u16 data = eeprom_read_word((u16*)(i * 2));
		melody[i - 1][0] = (u8)(data >> 8);
		melody[i - 1][1] = (u8)(data & 0x00FF);
	}
	CleanMelody();
	melody_index = 0;
}

void CleanMelody() {
	u8 is_finished = 0;
	for (u8 i = 0; i < melodyLimit + 1; i++) {
		if (is_finished) {
			melody[i][0] = 0x00;
			melody[i][1] = 0x00;
		} else {
			if (melody[i][1] == 0x00) {
				is_finished = 1;
			}
		}
	}
}