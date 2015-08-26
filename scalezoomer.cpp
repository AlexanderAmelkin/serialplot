/*
  Copyright © 2015 Hasan Yavuz Özderya

  This file is part of serialplot.

  serialplot is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  serialplot is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with serialplot.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QRectF>
#include <QtDebug>
#include "scalezoomer.h"

ScaleZoomer::ScaleZoomer(QwtPlot* plot, QwtPlotZoomer* zoomer) :
    QObject(plot),
    bottomPicker(plot->axisWidget(QwtPlot::xBottom)),
    leftPicker(plot->axisWidget(QwtPlot::yLeft))
{
    _plot = plot;
    _zoomer = zoomer;
    connect(&bottomPicker, &ScalePicker::pickStarted, this, &ScaleZoomer::bottomPickStarted);
    connect(&bottomPicker, &ScalePicker::picking, this, &ScaleZoomer::bottomPicking);
    connect(&bottomPicker, &ScalePicker::picked, this, &ScaleZoomer::bottomPicked);
    connect(&leftPicker, &ScalePicker::picked, this, &ScaleZoomer::leftPicked);
}

void ScaleZoomer::bottomPickStarted(double firstPos)
{
    double yMin = _plot->axisScaleDiv(QwtPlot::yLeft).lowerBound();
    double yMax = _plot->axisScaleDiv(QwtPlot::yLeft).upperBound();
    rectShape.setRect(QRectF(firstPos, yMin, 2, yMax-yMin));
    rectShape.attach(_plot);
    _plot->replot();
}

void ScaleZoomer::bottomPicking(double firstPos, double lastPos)
{
    double yMin = _plot->axisScaleDiv(QwtPlot::yLeft).lowerBound();
    double yMax = _plot->axisScaleDiv(QwtPlot::yLeft).upperBound();
    if (lastPos > firstPos) {
        rectShape.setRect(QRectF(firstPos, yMin, lastPos-firstPos, yMax-yMin));
    }
    else
    {
        rectShape.setRect(QRectF(lastPos, yMin, firstPos-lastPos, yMax-yMin));
    }
    _plot->replot();
}

void ScaleZoomer::bottomPicked(double firstPos, double lastPos)
{
    QRectF zRect;
    if (lastPos > firstPos)
    {
        zRect.setLeft(firstPos);
        zRect.setRight(lastPos);
    }
    else
    {
        zRect.setLeft(lastPos);
        zRect.setRight(firstPos);
    }

    zRect.setBottom(_plot->axisScaleDiv(QwtPlot::yLeft).lowerBound());
    zRect.setTop(_plot->axisScaleDiv(QwtPlot::yLeft).upperBound());
    _zoomer->zoom(zRect);
    rectShape.detach();
    _plot->replot();
}

void ScaleZoomer::leftPicked(double firstPos, double lastPos)
{
    QRectF zRect;
    if (lastPos > firstPos)
    {
        zRect.setBottom(firstPos);
        zRect.setTop(lastPos);
    }
    else
    {
        zRect.setBottom(lastPos);
        zRect.setTop(firstPos);
    }

    zRect.setLeft(_plot->axisScaleDiv(QwtPlot::xBottom).lowerBound());
    zRect.setRight(_plot->axisScaleDiv(QwtPlot::xBottom).upperBound());
    _zoomer->zoom(zRect);
}