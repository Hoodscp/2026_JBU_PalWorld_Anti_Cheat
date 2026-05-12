#pragma once

// Friendship / ArenaRankPoint 강제 채움.
// 둘 다 SaveParameter 의 int32 멤버라 매 프레임 단순 write 로 충분.
namespace SocialCheats
{
    void Tick();
}
