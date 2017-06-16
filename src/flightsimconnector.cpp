/* Copyright (C) 2017 Benjamin Isbarn.

   This file is part of the program BeneOverlay.

   BeneOverlay is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   BeneOverlay is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with BeneOverlay.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QDebug>
#include <QTimer>
#include "flightsimconnector.h"
#include <windows.h>
#include "FSUIPC_User.h"
#include <cstdint>

FlightSimConnector::FlightSimConnector(QObject *parent) : QObject(parent), _connected(false), _data_rate(1)
{
    _connect_fs_timer = new QTimer(this);
    _poll_data_timer = new QTimer(this);

    connect(_connect_fs_timer, SIGNAL(timeout()), this, SLOT(connectFS()));
    connect(_poll_data_timer, SIGNAL(timeout()), this, SLOT(pollData()));
}

FlightSimConnector::~FlightSimConnector()
{
    disconnectFS();

    qDebug() << "Disconnected";

}

bool FlightSimConnector::connected() const
{
    return _connected;
}

void FlightSimConnector::setConnected(bool connected)
{
    if (_connected != connected) {
        _connected = connected;
        emit connectedChanged(_connected);
    }
}

void FlightSimConnector::start()
{
    _connect_fs_timer->start(1000);
}

void FlightSimConnector::quit()
{
    _poll_data_timer->stop();
    _connect_fs_timer->stop();
    FSUIPC_Close();
    emit finished();
}

void FlightSimConnector::connectFS()
{
    if (!_connected) {
        DWORD dwResult;
        if (FSUIPC_Open(SIM_ANY, &dwResult)) {
            setConnected(true);
            _connect_fs_timer->stop();
            _poll_data_timer->start(1000/_data_rate);
            qDebug() << "Connected";
        }
        else {
            qDebug() << "Connection not successfull";
        }
    }
    else {
        qDebug() << "Allready connected, skipping ...";
    }
}

void FlightSimConnector::disconnectFS()
{
    FSUIPC_Close();
}

void FlightSimConnector::pollData()
{

    int32_t ias = 0;
    if (pollValue<int32_t, 4>(0x2BC, ias)) {
        emit parsedIas(ias/128.0);
    } else {
        return;
    }

    int32_t gs = 0;
    if (pollValue<int32_t, 4>(0x2B4, gs)) {
        emit parsedGs(gs/65536.0*1.94384);
    } else {
        return;
    }

    int32_t hdg = 0;
    if (pollValue<int32_t, 4>(0x580, hdg)) {

        emit parsedHdg(hdg >= 0.0? hdg*360.0/(65536.0*65536.0) : 360.0 + hdg*360.0/(65536.0*65536.0));
    } else {
        return;
    }

    int32_t altitude = 0;
    if (pollValue<int32_t, 4>(0x3324, altitude)) {
        emit parsedAltitude(altitude);
    } else {
        return;
    }

    int32_t vs = 0;
    if (pollValue<int32_t, 4>(0x2C8, vs)) {
        emit parsedVs(vs / 256.0 * 60 * 3.28084);
    } else {
        return;
    }

    int16_t wind_dir = 0;
    if (pollValue<int16_t, 2>(0xE92, wind_dir)) {
        emit parsedWindDir(wind_dir >= 0.0? wind_dir * 360.0 / 65536.0 : 360.0 + wind_dir * 360.0 / 65536.0);
    } else {
        return;
    }

    int16_t wind_mag = 0;
    if (pollValue<int16_t, 2>(0xE90, wind_mag)) {
        emit parsedWindMag(wind_mag);
    } else {
        return;
    }

    int32_t vas_usage = 0;
    if (pollValue<int32_t, 4>(0x24C, vas_usage)) {
        // Free VAS in MB
        emit parsedVasUsage(vas_usage/1024);
    } else {
        return;
    }

    int64_t latitude = 0;
    if (pollValue<int64_t, 8>(0x0560, latitude)) {
        double converted_latitude = latitude * 90.0 / (10001750.0 * 65536.0 * 65536.0);
        emit parsedLatitude(converted_latitude);
    } else {
        return;
    }

    int64_t longitude = 0;
    if (pollValue<int64_t, 8>(0x0568, longitude)) {
        double converted_longitude = longitude * 360.0 / (65536.0 * 65536.0 * 65536.0 * 65536.0);
        emit parsedLongitude(converted_longitude);
    } else {
        return;
    }

    int16_t fps = 0;
    if (pollValue<int16_t, 2>(0x0274, fps)) {
        if (fps != 0) {
            emit parsedFps(32768 / fps);
        } else {
            emit parsedFps(0);
        }
    } else {
        return;
    }

    int32_t gear = 0;
    if (pollValue<int32_t, 4>(0x0BE8, gear)) {
        emit parsedGearDown(gear);
    } else {
        return;
    }

    int32_t pitch = 0;
    if (pollValue<int32_t, 4>(0x0578, pitch)) {
        emit parsedPitch(- pitch * 360.0 / (65536.0*65536.0));
    } else {
        return;
    }

    int32_t vs_air = 0;
    if (pollValue<int32_t, 4>(0x030C, vs_air)) {
        emit parsedVsAir(vs_air / 256.0 * 60 * 3.28084);
    } else {
        return;
    }

    int32_t bank = 0;
    if (pollValue<int32_t, 4>(0x057C, bank)) {
        emit parsedBank(bank * 360.0 / (65536.0 * 65536.0));
    } else {
        return;
    }

    int16_t on_ground = 0;
    if (pollValue<int16_t, 2>(0x0366, on_ground)) {
        emit parsedOnGround(on_ground);
    } else {
        return;
    }

    int16_t oat = 0;
    if (pollValue<int16_t, 2>(0x0e8c, oat)) {
        emit parsedOat(oat / 256.0);
    } else {
        return;
    }

    int16_t tat = 0;
    if (pollValue<int16_t, 2>(0x11d0, tat)) {
        emit parsedTat(tat / 256.0);
    } else {
        return;
    }
}

void FlightSimConnector::setDataRate(const QVariant &data_rate)
{
    if (_data_rate != data_rate.toInt() && data_rate.toInt() > 0) {
        _data_rate = data_rate.toInt();
        _poll_data_timer->setInterval(1000/_data_rate);
    }

}

template <typename T, size_t n>
bool FlightSimConnector::FlightSimConnector::pollValue(int offset, T &value)
{
    DWORD dw_result;
    T tmp_value;
    if (!FSUIPC_Read(offset, n, &tmp_value, &dw_result) || !FSUIPC_Process(&dw_result)) {
        qDebug() << "Error processing request: " << dw_result;
        qDebug() << "Reconnecting ...";
        _poll_data_timer->stop();
        FSUIPC_Close();
        setConnected(false);
        _connect_fs_timer->start(1000);
        return false;
    }
    value = tmp_value;
    return true;
}
