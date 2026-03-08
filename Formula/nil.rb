class Nil < Formula
  desc "TTY-based lyrics viewer"
  homepage "https://github.com/nurislamaibekuly/nil"
  
  if Hardware::CPU.intel?
    url "https://github.com/nurislamaibekuly/nil/releases/download/stable/nil-macos-x86_64"
    sha256 "b15f366c2eeb48e34e282a8cd7772896ee238ee8b21cf60a7e6e6d950ad02e77"
  else
    url "https://github.com/nurislamaibekuly/nil/releases/download/stable/nil-macos-aarch64"
    sha256 "06b24adab3bfe43480bd3e2ed85e32c4c3bc3d985380ea6f49bb04a6d5797b2f"
  end

  def install
    bin.install (Hardware::CPU.intel? ? "nil-macos-x86_64" : "nil-macos-aarch64") => "nil"
  end

  test do
    assert_match "nil", shell_output("#{bin}/nil --version")
  end
end