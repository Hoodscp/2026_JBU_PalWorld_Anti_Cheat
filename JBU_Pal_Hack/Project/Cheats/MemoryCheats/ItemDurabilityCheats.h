#pragma once

// 장비 내구도 / 무기 잔탄 강제 치트.
// UPalItemSlot.DynamicItemData 는 TWeakObjectPtr 라 자동 해소가 어려워
// 사용자가 한 번 Cheat Engine 등으로 UPalDynamicArmorItemDataBase /
// UPalDynamicWeaponItemDataBase 인스턴스의 절대 주소를 잡아 등록해 두면
// 매 프레임 Durability/RemainingBullets 를 강제로 채워준다.
namespace ItemDurabilityCheats
{
    void Tick();
}
