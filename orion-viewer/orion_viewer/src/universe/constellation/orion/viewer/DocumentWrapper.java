/*
 * Orion Viewer - pdf, djvu, xps and cbz file viewer for android devices
 *
 * Copyright (C) 2011-2012  Michael Bogdanov
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
 */

package universe.constellation.orion.viewer;

import universe.constellation.orion.viewer.outline.OutlineItem;

/**
 * User: mike
 * Date: 15.10.11
 * Time: 9:53
 */
public interface DocumentWrapper {

    boolean openDocument(String fileName);

    int getPageCount();

    PageInfo getPageInfo(int pageNum);

    int[] renderPage(int pageNumber, double zoom, int w, int h, int left, int top, int right, int bottom);

    public void gotoPage(int page);

    String getText(int pageNumber, int absoluteX, int absoluteY, int width, int height);

	void destroy();

    String getTitle();

	void setContrast(int contrast);

	void setThreshold(int threshold);

    void setReflow(int reflow);

    void setReflowParameters(float zoom,
                             int dpi,
                             int columns,
                             int bb_width,
                             int bb_height,
                             int m_top,
                             int m_bottom,
                             int m_left,
                             int m_right,
                             int default_trim,
                             int wrap_text,
                             int indent,
                             int rotation,
                             float margin,
                             float word_space,
                             float quality,
                             int ocr_language,
                             int white_thresh);

	public OutlineItem[] getOutline();
}
