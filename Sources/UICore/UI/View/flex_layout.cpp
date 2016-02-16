/*
**  UICore
**  Copyright (c) 1997-2015 The UICore Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries UICore may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Magnus Norddahl
*/

#include "UICore/precomp.h"
#include "flex_layout.h"
#include <algorithm>
#include <cmath>

namespace uicore
{
	float FlexLayout::preferred_width(const CanvasPtr &canvas, View *view)
	{
		calculate_layout(canvas, view);

		if (direction == FlexDirection::row)
		{
			float width = 0.0f;
			for (auto &line : lines)
			{
				width = std::max(width, line.total_main_noncontent + line.total_flex_preferred_main_size);
			}
			return width;
		}
		else
		{
			float width = 0.0f;
			for (auto &line : lines)
			{
				width += line.cross_size;
			}
			return width;
		}
	}

	float FlexLayout::preferred_height(const CanvasPtr &canvas, View *view, float width)
	{
		calculate_layout(canvas, view);

		if (direction == FlexDirection::row)
		{
			float height = 0.0f;
			for (auto &line : lines)
			{
				height += line.cross_size;
			}
			return height;
		}
		else
		{
			float height = 0.0f;
			for (auto &line : lines)
			{
				height = std::max(height, line.total_main_noncontent + line.total_flex_preferred_main_size);
			}
			return height;
		}
	}

	float FlexLayout::first_baseline_offset(const CanvasPtr &canvas, View *view, float width)
	{
		return 0.0f;
	}

	float FlexLayout::last_baseline_offset(const CanvasPtr &canvas, View *view, float width)
	{
		return 0.0f;
	}

	void FlexLayout::layout_subviews(const CanvasPtr &canvas, View *view)
	{
		calculate_layout(canvas, view);

		if (direction == FlexDirection::row)
		{
			for (auto &line : lines)
			{
				for (auto &item : line)
				{
					Rectf box = Rectf::xywh(item.used_main_pos, item.used_cross_pos, item.used_main_size, item.used_cross_size);
					item.view->set_geometry(ViewGeometry::from_content_box(item.view->style_cascade(), box));
					item.view->layout_subviews(canvas);
				}
			}
		}
		else
		{
			for (auto &line : lines)
			{
				for (auto &item : line)
				{
					Rectf box = Rectf::xywh(item.used_cross_pos, item.used_main_pos, item.used_cross_size, item.used_main_size);
					item.view->set_geometry(ViewGeometry::from_content_box(item.view->style_cascade(), box));
					item.view->layout_subviews(canvas);
				}
			}
		}
	}

	void FlexLayout::calculate_layout(const CanvasPtr &canvas, View *view)
	{
		create_items(canvas, view);
		create_lines(canvas, view);
		flex_lines(canvas, view);
		calculate_items_preferred_cross_size(canvas, view);
		calculate_lines_cross_size(canvas, view);
		main_axis_alignment(canvas, view);
		cross_axis_alignment(canvas, view);
	}

	void FlexLayout::create_items(const CanvasPtr &canvas, View *view)
	{
		const auto &container_style = view->style_cascade();

		auto computed_direction = container_style.computed_value("flex-direction");
		auto computed_wrap = container_style.computed_value("flex-wrap");

		direction = computed_direction.is_keyword("row") ? FlexDirection::row : FlexDirection::column;

		if (computed_wrap.is_keyword("nowrap"))
			wrap = FlexWrap::nowrap;
		else if (computed_wrap.is_keyword("wrap"))
			wrap = FlexWrap::nowrap;
		else if (computed_wrap.is_keyword("wrap-reverse"))
			wrap = FlexWrap::nowrap;

		container_main_size = (direction == FlexDirection::row) ? view->geometry().content_width : view->geometry().content_height;
		container_cross_size = (direction == FlexDirection::column) ? view->geometry().content_width : view->geometry().content_height;

		if (direction == FlexDirection::row)
			create_row_items(canvas, view);
		else
			create_column_items(canvas, view);
	}

	void FlexLayout::create_row_items(const CanvasPtr &canvas, View *view)
	{
		items.clear();

		for (const std::shared_ptr<View> &subview : view->subviews())
		{
			if (!subview->is_static_position_and_visible())
				continue;

			const auto &item_style = subview->style_cascade();

			FlexLayoutItem item;
			item.view = subview.get();

			// Definite sizes:

			item.definite_main_size = item_style.computed_value("width").is_length();
			item.definite_cross_size = item_style.computed_value("height").is_length();
			item.definite_min_main_size = item_style.computed_value("min-width").is_length();
			item.definite_max_main_size = item_style.computed_value("max-width").is_length();
			item.definite_min_cross_size = item_style.computed_value("min-height").is_length();
			item.definite_max_cross_size = item_style.computed_value("max-height").is_length();

			if (item.definite_main_size)
				item.main_size = item_style.computed_value("width").number();
			if (item.definite_cross_size)
				item.cross_size = item_style.computed_value("height").number();
			if (item.definite_min_main_size)
				item.min_main_size = item_style.computed_value("min-width").number();
			if (item.definite_max_main_size)
				item.max_main_size = item_style.computed_value("max-width").number();
			if (item.definite_min_cross_size)
				item.min_cross_size = item_style.computed_value("min-height").number();
			if (item.definite_max_cross_size)
				item.max_cross_size = item_style.computed_value("max-height").number();

			// Main axis auto min:

			if (item_style.computed_value("min-width").is_keyword("auto"))
			{
				float min_content_size = 0.0f; // item.view->min_content_width(); // shrink-to-fit in CSS 2.1
				if (item.definite_main_size)
				{
					float specified_size = item.main_size;
					if (item.definite_max_main_size)
						specified_size = std::min(specified_size, item.max_main_size);
					item.min_main_size = std::min(specified_size, min_content_size);
				}
				else if (item.definite_cross_size /*&& item.view->intrinsic_aspect_ratio() != 0.0f*/)
				{
					float clamped_cross_size = item.cross_size;

					if (item.definite_min_cross_size)
						clamped_cross_size = std::max(clamped_cross_size, item.min_cross_size);

					if (item.definite_max_cross_size)
						clamped_cross_size = std::min(clamped_cross_size, item.max_cross_size);

					float aspect = 1.0f; // item.view->intrinsic_aspect_ratio();
					float transfered_size = clamped_cross_size / aspect;
					item.min_main_size = std::min(transfered_size, min_content_size);
				}
				else
				{
					item.min_main_size = min_content_size;
				}
			}

			// Non-content sizes:

			item.main_noncontent_start += subview->style_cascade().computed_value("margin-left").number();
			item.main_noncontent_start += subview->style_cascade().computed_value("border-left-width").number();
			item.main_noncontent_start += subview->style_cascade().computed_value("padding-left").number();
			item.main_noncontent_end += subview->style_cascade().computed_value("padding-right").number();
			item.main_noncontent_end += subview->style_cascade().computed_value("border-right-width").number();
			item.main_noncontent_end += subview->style_cascade().computed_value("margin-right").number();

			item.main_auto_margin_start = item_style.computed_value("margin-left").is_keyword("auto");
			item.main_auto_margin_end = item_style.computed_value("margin-right").is_keyword("auto");

			item.cross_noncontent_start += subview->style_cascade().computed_value("margin-top").number();
			item.cross_noncontent_start += subview->style_cascade().computed_value("border-top-width").number();
			item.cross_noncontent_start += subview->style_cascade().computed_value("padding-top").number();
			item.cross_noncontent_end += subview->style_cascade().computed_value("padding-bottom").number();
			item.cross_noncontent_end += subview->style_cascade().computed_value("border-bottom-width").number();
			item.cross_noncontent_end += subview->style_cascade().computed_value("margin-bottom").number();

			item.cross_auto_margin_start = item_style.computed_value("margin-top").is_keyword("auto");
			item.cross_auto_margin_end = item_style.computed_value("margin-bottom").is_keyword("auto");

			// Flex base size and hypothetical (preferred) main size:

			if (item.view->style_cascade().computed_value("flex-basis").is_length())
				item.flex_base_size = item.view->style_cascade().computed_value("flex-basis").number();
			else if (item.definite_main_size)
				item.flex_base_size = item.main_size;
			else
				item.flex_base_size = item.view->preferred_width(canvas);

			item.flex_preferred_main_size = item.flex_base_size;

			if (item.definite_min_main_size)
				item.flex_preferred_main_size = std::max(item.flex_preferred_main_size, item.min_main_size);

			if (item.definite_max_main_size)
				item.flex_preferred_main_size = std::min(item.flex_preferred_main_size, item.max_main_size);

			item.flex_grow = item.view->style_cascade().computed_value("flex-grow").number();
			item.flex_shrink = item.view->style_cascade().computed_value("flex-shrink").number();

			items.push_back(item);
		}
	}

	void FlexLayout::create_column_items(const CanvasPtr &canvas, View *view)
	{
		items.clear();

		for (const std::shared_ptr<View> &subview : view->subviews())
		{
			if (!subview->is_static_position_and_visible())
				continue;

			const auto &item_style = subview->style_cascade();

			FlexLayoutItem item;
			item.view = subview.get();

			// Definite sizes:

			item.definite_main_size = item_style.computed_value("height").is_length();
			item.definite_cross_size = item_style.computed_value("width").is_length();
			item.definite_min_main_size = item_style.computed_value("min-height").is_length();
			item.definite_max_main_size = item_style.computed_value("max-height").is_length();
			item.definite_min_cross_size = item_style.computed_value("min-width").is_length();
			item.definite_max_cross_size = item_style.computed_value("max-width").is_length();

			if (item.definite_main_size)
				item.main_size = item_style.computed_value("height").number();
			if (item.definite_cross_size)
				item.cross_size = item_style.computed_value("width").number();
			if (item.definite_min_main_size)
				item.min_main_size = item_style.computed_value("min-height").number();
			if (item.definite_max_main_size)
				item.max_main_size = item_style.computed_value("max-height").number();
			if (item.definite_min_cross_size)
				item.min_cross_size = item_style.computed_value("min-width").number();
			if (item.definite_max_cross_size)
				item.max_cross_size = item_style.computed_value("max-width").number();

			// Main axis auto min:

			if (item_style.computed_value("min-height").is_keyword("auto"))
			{
				float min_content_size = 0.0f; // item.view->preferred_height(canvas, item.view->min_content_width()); // shrink-to-fit in CSS 2.1
				if (item.definite_main_size)
				{
					float specified_size = item.main_size;
					if (item.definite_max_main_size)
						specified_size = std::min(specified_size, item.max_main_size);
					item.min_main_size = std::min(specified_size, min_content_size);
				}
				else if (item.definite_cross_size /*&& item.view->intrinsic_aspect_ratio() != 0.0f*/)
				{
					float clamped_cross_size = item.cross_size;

					if (item.definite_min_cross_size)
						clamped_cross_size = std::max(clamped_cross_size, item.min_cross_size);

					if (item.definite_max_cross_size)
						clamped_cross_size = std::min(clamped_cross_size, item.max_cross_size);

					float aspect = 1.0f; // item.view->intrinsic_aspect_ratio();
					float transfered_size = clamped_cross_size * aspect;
					item.min_main_size = std::min(transfered_size, min_content_size);
				}
				else
				{
					item.min_main_size = min_content_size;
				}
			}

			// Non-content sizes:

			item.main_noncontent_start += subview->style_cascade().computed_value("margin-top").number();
			item.main_noncontent_start += subview->style_cascade().computed_value("border-top-width").number();
			item.main_noncontent_start += subview->style_cascade().computed_value("padding-top").number();
			item.main_noncontent_end += subview->style_cascade().computed_value("padding-bottom").number();
			item.main_noncontent_end += subview->style_cascade().computed_value("border-bottom-width").number();
			item.main_noncontent_end += subview->style_cascade().computed_value("margin-bottom").number();

			item.main_auto_margin_start = item_style.computed_value("margin-top").is_keyword("auto");
			item.main_auto_margin_end = item_style.computed_value("margin-bottom").is_keyword("auto");

			item.cross_noncontent_start += subview->style_cascade().computed_value("margin-left").number();
			item.cross_noncontent_start += subview->style_cascade().computed_value("border-left-width").number();
			item.cross_noncontent_start += subview->style_cascade().computed_value("padding-left").number();
			item.cross_noncontent_end += subview->style_cascade().computed_value("padding-right").number();
			item.cross_noncontent_end += subview->style_cascade().computed_value("border-right-width").number();
			item.cross_noncontent_end += subview->style_cascade().computed_value("margin-right").number();

			item.cross_auto_margin_start = item_style.computed_value("margin-left").is_keyword("auto");
			item.cross_auto_margin_end = item_style.computed_value("margin-right").is_keyword("auto");

			// Flex base size and hypothetical (preferred) main size:

			if (item.view->style_cascade().computed_value("flex-basis").is_length())
				item.flex_base_size = item.view->style_cascade().computed_value("flex-basis").number();
			else if (item.definite_main_size)
				item.flex_base_size = item.main_size;
			else if (item.definite_cross_size)
				item.flex_base_size = item.view->preferred_height(canvas, item.cross_size);
			else
				item.flex_base_size = item.view->preferred_height(canvas, std::max(view->geometry().content_width - item.cross_noncontent_start - item.cross_noncontent_end, 0.0f));

			item.flex_preferred_main_size = item.flex_base_size;

			if (item.definite_min_main_size)
				item.flex_preferred_main_size = std::max(item.flex_preferred_main_size, item.min_main_size);

			if (item.definite_max_main_size)
				item.flex_preferred_main_size = std::min(item.flex_preferred_main_size, item.max_main_size);

			item.flex_grow = item.view->style_cascade().computed_value("flex-grow").number();
			item.flex_shrink = item.view->style_cascade().computed_value("flex-shrink").number();

			items.push_back(item);
		}
	}

	void FlexLayout::create_lines(const CanvasPtr &canvas, View *view)
	{
		lines.clear();

		if (wrap == FlexWrap::nowrap || infinite_container_main_size) // single-line
		{
			FlexLayoutLine line(items.begin(), items.end());

			for (auto &item : items)
			{
				if (restarted_layout && item.collapsed)
					continue;

				line.total_flex_preferred_main_size += item.flex_preferred_main_size;
				line.total_main_noncontent += item.main_noncontent_start + item.main_noncontent_end;
			}

			lines.push_back(line);
		}
		else
		{
			FlexLayoutLine line(items.begin(), items.end());

			for (auto it = items.begin(); it != items.end(); ++it)
			{
				auto &item = *it;

				if (restarted_layout && item.collapsed)
					continue;

				float item_size = item.main_noncontent_start + item.flex_preferred_main_size + item.main_noncontent_end;
				float pos = line.total_main_noncontent + line.total_flex_preferred_main_size + item_size;

				if (line.begin() != it && pos > container_main_size && item_size > 0.0f)
				{
					line.set_end(it);
					lines.push_back(line);
					line = FlexLayoutLine(it, items.end());
				}

				line.total_flex_preferred_main_size += item.flex_preferred_main_size;
				line.total_main_noncontent += item.main_noncontent_start + item.main_noncontent_end;
			}

			lines.push_back(line);
		}
	}

	void FlexLayout::flex_lines(const CanvasPtr &canvas, View *view)
	{
		for (auto &line : lines)
		{
			int unfrozen_count = 0;
			bool use_grow_factor = (line.total_flex_preferred_main_size + line.total_main_noncontent < container_main_size);

			// Size inflexible items and calculate initial free space:

			float initial_space_needed = 0.0f;
			if (use_grow_factor)
			{
				for (auto &item : line)
				{
					if (restarted_layout && item.collapsed)
					{
						item.used_main_size = 0.0f;
						item.frozen = true;
					}
					else if (item.flex_grow == 0.0f || item.flex_base_size > item.flex_preferred_main_size)
					{
						item.used_main_size = item.flex_preferred_main_size;
						item.frozen = true;
						initial_space_needed += item.flex_preferred_main_size;
					}
					else
					{
						item.frozen = false;
						initial_space_needed += item.flex_base_size;
						unfrozen_count++;
					}
				}
			}
			else
			{
				for (auto &item : line)
				{
					if (restarted_layout && item.collapsed)
					{
						item.used_main_size = 0.0f;
						item.frozen = true;
					}
					else if (item.flex_shrink == 0.0f || item.flex_base_size < item.flex_preferred_main_size)
					{
						item.used_main_size = item.flex_preferred_main_size;
						item.frozen = true;
						initial_space_needed += item.flex_preferred_main_size;
					}
					else
					{
						item.frozen = false;
						initial_space_needed += item.flex_base_size;
						unfrozen_count++;
					}
				}
			}

			float initial_free_space = container_main_size - initial_space_needed;

			// Flex remaining items:

			while (unfrozen_count > 0)
			{
				// Calculate remaining free space:

				float remaining_space_needed = 0.0f;
				float flex_factor_sum = 0.0f;
				for (auto &item : line)
				{
					if (!item.frozen)
					{
						remaining_space_needed += item.flex_base_size;
						flex_factor_sum += use_grow_factor ? item.flex_grow : item.flex_shrink;
					}
				}

				float remaining_free_space = container_main_size - remaining_space_needed;

				if (flex_factor_sum < 1.0f)
				{
					float value = flex_factor_sum * initial_free_space;
					if (std::abs(value) < std::abs(remaining_free_space))
						remaining_free_space = value;
				}

				// Distribute free space:

				if (use_grow_factor)
				{
					for (auto &item : line)
					{
						if (!item.frozen)
						{
							item.used_main_size = item.flex_base_size + remaining_free_space * (item.flex_grow / flex_factor_sum);
						}
					}
				}
				else
				{
					float scaled_flex_factor_sum = 0.0f;
					for (auto &item : line)
					{
						if (!item.frozen)
						{
							item.scaled_flex_shrink = item.flex_shrink * item.flex_base_size;
							scaled_flex_factor_sum += item.scaled_flex_shrink;
						}
					}

					for (auto &item : line)
					{
						if (!item.frozen)
						{
							item.used_main_size = item.flex_base_size - std::abs(remaining_free_space * (item.scaled_flex_shrink / scaled_flex_factor_sum));
						}
					}
				}

				// Fix min/max violations:

				float total_violation = 0.0f;
				for (auto &item : line)
				{
					if (!item.frozen)
					{
						float unclamped_size = item.used_main_size;
						float clamped_size = unclamped_size;

						if (item.definite_min_main_size)
							clamped_size = std::max(clamped_size, item.min_main_size);

						if (item.definite_max_main_size)
							clamped_size = std::min(clamped_size, item.max_main_size);

						if (clamped_size < unclamped_size)
							item.flex_violation = FlexViolation::max_violation;
						else if (clamped_size > unclamped_size)
							item.flex_violation = FlexViolation::min_violation;
						else
							item.flex_violation = FlexViolation::none;

						item.used_main_size = clamped_size;

						total_violation += clamped_size - unclamped_size;
					}
				}

				// Freeze over-flexed items or all items if there were no violations:

				for (auto &item : line)
				{
					if (!item.frozen)
					{
						bool freeze = 
							(total_violation >= 0.0f && item.flex_violation == FlexViolation::max_violation) ||
							(total_violation <= 0.0f && item.flex_violation == FlexViolation::min_violation);
						if (freeze)
						{
							item.frozen = true;
							unfrozen_count--;
						}
					}
				}
			}
		}
	}

	void FlexLayout::calculate_items_preferred_cross_size(const CanvasPtr &canvas, View *view)
	{
		for (auto &line : lines)
		{
			for (auto &item : line)
			{
				if (restarted_layout && item.collapsed)
					continue;

				if (item.definite_cross_size)
					item.flex_preferred_cross_size = item.cross_size;
				else if (direction == FlexDirection::row)
					item.flex_preferred_cross_size = item.view->preferred_height(canvas, item.used_main_size);
				else
					item.flex_preferred_cross_size = item.view->preferred_width(canvas/*, item.used_main_size*/);

				// Note: This clamping is not specified in the CSS-flexbox-1 "Cross Size Determination" section
				//
				// It might because the min/max clamping for CSS 2.1 is somehow more complex and implicitly understood to be done
				// by the remarked "baseline offset" calculation code in the calculate_lines_cross_size function below.
				//
				// Maybe by 2037 when W3C actually finished the CSS 3 specifications it will be more clear why this is needed for HTML..

				if (item.definite_min_cross_size)
				item.flex_preferred_cross_size = std::max(item.flex_preferred_cross_size, item.min_cross_size);

				if (item.definite_max_cross_size)
				item.flex_preferred_cross_size = std::min(item.flex_preferred_cross_size, item.max_cross_size);
			}
		}
	}

	void FlexLayout::calculate_lines_cross_size(const CanvasPtr &canvas, View *view)
	{
		if (wrap == FlexWrap::nowrap && definite_container_cross_size)
		{
			for (auto &line : lines)
			{
				line.cross_size = container_cross_size;

				if (restarted_layout)
				{
					for (auto &item : line)
					{
						if (item.collapsed)
							line.cross_size = std::max(line.cross_size, item.strut_size);
					}
				}
			}
		}
		else
		{
			for (auto &line : lines)
			{
				line.cross_size = 0.0f;
				for (auto &item : line)
				{
					if (restarted_layout && item.collapsed)
						line.cross_size = std::max(line.cross_size, item.strut_size);
					else
						line.cross_size = std::max(line.cross_size, item.cross_noncontent_start + item.flex_preferred_cross_size + item.cross_noncontent_end);
				}

#if 0
				// See note in calculate_items_preferred_cross_size function for why this is commented out and replaced by the above simplified code. 

				float max_start_outer_baseline_offset = 0.0f;
				float max_end_outer_baseline_offset = 0.0f;
				float max_outer_preferred_cross_size = 0.0f;

				for (auto &item : line)
				{
					auto &item_style = item.view->style_cascade();
					if (restarted_layout && item.collapsed)
					{
						max_outer_preferred_cross_size = std::max(max_outer_preferred_cross_size, item.strut_size);
					}
					else if (direction == FlexDirection::row && item_style.computed_value("align-self").is_keyword("baseline") && !item.cross_auto_margin_start && !item.cross_auto_margin_end)
					{
						float baseline_offset = item.view->first_baseline_offset(canvas, item.used_main_size);
						float start_outer_baseline_offset = item.cross_noncontent_start + baseline_offset;
						float end_outer_baseline_offset = item.flex_preferred_cross_size - baseline_offset + item.cross_noncontent_end;

						max_start_outer_baseline_offset = std::max(max_start_outer_baseline_offset, start_outer_baseline_offset);
						max_end_outer_baseline_offset = std::max(max_end_outer_baseline_offset, end_outer_baseline_offset);
					}
					else
					{
						max_outer_preferred_cross_size = std::max(max_outer_preferred_cross_size, item.cross_noncontent_start + item.flex_preferred_cross_size + item.cross_noncontent_end);
					}
				}

				float max_outer_baseline_cross_size = max_start_outer_baseline_offset + max_end_outer_baseline_offset;

				line.cross_size = std::max(max_outer_preferred_cross_size, max_outer_baseline_cross_size);
#endif
			}
		}

		if (view->style_cascade().computed_value("align-content").is_keyword("stretch") && definite_container_cross_size)
		{
			float total_cross_size = 0.0f;
			for (auto &line : lines)
				total_cross_size += line.cross_size;

			float free_space = (container_cross_size - total_cross_size) / (float)lines.size();
			if (free_space > 0.0f)
			{
				for (auto &line : lines)
					line.cross_size += free_space;
			}
		}

		if (restarted_layout)
			return;

		for (auto &line : lines)
		{
			for (auto &item : line)
			{
				if (item.view->style_cascade().computed_value("visibility").is_keyword("collapse"))
				{
					item.collapsed = true;
					item.strut_size = line.cross_size;
					restarted_layout = true;
				}
			}
		}

		if (restarted_layout)
		{
			create_lines(canvas, view);
			flex_lines(canvas, view);
			calculate_items_preferred_cross_size(canvas, view);
			calculate_lines_cross_size(canvas, view);
			restarted_layout = false;
		}

		for (auto &line : lines)
		{
			for (auto &item : line)
			{
				auto &item_style = item.view->style_cascade();
				if (item_style.computed_value("align-self").is_keyword("stretch") && !item.definite_cross_size && !item.cross_auto_margin_start && !item.cross_auto_margin_end)
				{
					item.used_cross_size = line.cross_size - item.cross_noncontent_start - item.cross_noncontent_end;

					if (item.definite_min_cross_size)
						item.used_cross_size = std::max(item.used_cross_size, item.min_cross_size);

					if (item.definite_max_cross_size)
						item.used_cross_size = std::min(item.used_cross_size, item.max_cross_size);
				}
				else
				{
					item.used_cross_size = item.flex_preferred_cross_size;
				}

#if 0 // We do not support percentage sizing at the moment
				if (item_style.computed_value("align-self").is_keyword("stretch"))
				{
					// redo layout for its contents, treating this used size as its definite cross size so that percentage-sized children can be resolved:

					//item.view->set_geometry_width_definite();
					//item.view->set_geometry_height_definite();
				}
#endif
			}
		}
	}

	void FlexLayout::main_axis_alignment(const CanvasPtr &canvas, View *view)
	{
		for (auto &line : lines)
		{
			int auto_margin_count = 0;
			int item_count = 0;
			float space_consumed = 0;
			for (auto &item : line)
			{
				if (item.collapsed)
					continue;

				if (item.main_auto_margin_start)
					auto_margin_count++;
				if (item.main_auto_margin_end)
					auto_margin_count++;

				space_consumed += item.main_noncontent_start;
				space_consumed += item.used_main_size;
				space_consumed += item.main_noncontent_end;
				item_count++;
			}

			float space_available = container_main_size - space_consumed;

			if (auto_margin_count > 0 && space_available > 0.0f)
			{
				float space_per_auto_margin = space_available / auto_margin_count;
				for (auto &item : line)
				{
					if (item.main_auto_margin_start)
						item.main_noncontent_start += space_per_auto_margin;
					if (item.main_auto_margin_end)
						item.main_noncontent_end += space_per_auto_margin;
				}
				space_available = 0.0f;
			}

			auto justify_content = view->style_cascade().computed_value("justify-content");
			if (justify_content.is_keyword("flex-start") || ((item_count < 2 || space_available < 0.0f) && justify_content.is_keyword("space-between")))
			{
				float pos = 0.0f;
				for (auto &item : line)
				{
					if (item.collapsed)
						continue;

					pos += item.main_noncontent_start;
					item.used_main_pos = pos;
					pos += item.used_main_size;
					pos += item.main_noncontent_end;
				}
			}
			else if (justify_content.is_keyword("flex-end"))
			{
				float pos = container_main_size;
				auto it = line.begin();
				while (it != line.end())
				{
					auto &item = *(++it);
					if (item.collapsed)
						continue;

					pos -= item.main_noncontent_end;
					pos -= item.used_main_size;
					item.used_main_pos = pos;
					pos -= item.main_noncontent_start;
				}
			}
			else if (justify_content.is_keyword("center") || ((item_count < 2 || space_available < 0.0f) && justify_content.is_keyword("space-around")))
			{
				float pos = (container_main_size - space_consumed) / 2.0f;
				for (auto &item : line)
				{
					if (item.collapsed)
						continue;

					pos += item.main_noncontent_start;
					item.used_main_pos = pos;
					pos += item.used_main_size;
					pos += item.main_noncontent_end;
				}
			}
			else if (justify_content.is_keyword("space-between"))
			{
				float space_extra = space_available / (float)(item_count - 1);
				float pos = 0.0f;
				for (auto &item : line)
				{
					if (item.collapsed)
						continue;

					pos += item.main_noncontent_start;
					item.used_main_pos = pos;
					pos += item.used_main_size;
					pos += item.main_noncontent_end;

					pos += space_extra;
				}
			}
			else if (justify_content.is_keyword("space-around"))
			{
				float space_extra = space_available / (float)(item_count - 1);
				float pos = space_extra / 2.0f;
				for (auto &item : line)
				{
					if (item.collapsed)
						continue;

					pos += item.main_noncontent_start;
					item.used_main_pos = pos;
					pos += item.used_main_size;
					pos += item.main_noncontent_end;

					pos += space_extra;
				}
			}
		}
	}

	void FlexLayout::cross_axis_alignment(const CanvasPtr &canvas, View *view)
	{
	}

}