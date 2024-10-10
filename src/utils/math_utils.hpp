#pragma once

template <typename T>
T mapRange(T input, T input_start, T input_end, T output_start, T output_end)
{
	return (output_start + ((output_end - output_start) / (input_end - input_start)) * (input - input_start));
}
