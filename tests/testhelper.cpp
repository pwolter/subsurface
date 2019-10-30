// SPDX-License-Identifier: GPL-2.0
#include "testhelper.h"
#include "core/btdiscovery.h"

void TestHelper::initTestCase()
{
	/* we need to manually tell that the resource exists, because we are using it as library. */
	Q_INIT_RESOURCE(subsurface);
}

void TestHelper::recognizeBtAddress()
{
	QCOMPARE(isBluetoothAddress("01:a2:b3:c4:d5:06"), true);
	QCOMPARE(isBluetoothAddress("LE:01:A2:B3:C4:D5:06"), true);
	QCOMPARE(isBluetoothAddress("01:A2:b3:04:05"), false);
	QCOMPARE(isBluetoothAddress("LE:01:02:03:04:05"), false);
	QCOMPARE(isBluetoothAddress("01:02:03:04:051:67"), false);
	QCOMPARE(isBluetoothAddress("LE:01:g2:03:04:05"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), true);
	QCOMPARE(isBluetoothAddress("LE:{6e5ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdda33-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-ag0a-1ed4c7d2d2a5}"), false);
}

void TestHelper::parseNameAddress()
{
	QString name, address;
	address = extractBluetoothNameAddress("01:a2:b3:c4:d5:06", name);
	QCOMPARE(address, QString("01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString());
	address = extractBluetoothNameAddress("somename (01:a2:b3:c4:d5:06)", name);
	QCOMPARE(address, QString("01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString("somename"));
	address = extractBluetoothNameAddress("garbage", name);
	QCOMPARE(address, QString());
	QCOMPARE(name, QString());
	address = extractBluetoothNameAddress("somename (LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5})", name);
	QCOMPARE(address, QString("LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"));
	QCOMPARE(name, QString("somename"));

}

QTEST_GUILESS_MAIN(TestHelper)
