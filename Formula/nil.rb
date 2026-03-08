class Nil < Formula
  desc "TTY-based lyrics viewer"
  homepage "https://github.com/nurislamaibekuly/nil"
  
  if Hardware::CPU.intel?
    url "https://github.com/nurislamaibekuly/nil/releases/download/stable/nil-macos-x86_64"
    sha256 "e475d9d3def5e2bc8397620fa9b5708456661e26c8924148daaf0163dd948b40"
  else
    url "https://github.com/nurislamaibekuly/nil/releases/download/stable/nil-macos-aarch64"
    sha256 "f879763a47b7dce58bb0a0fde54f86e5f6333b2546c66ae1c0efb0ae758c95e9"
  end

  def install
    bin.install (Hardware::CPU.intel? ? "nil-macos-x86_64" : "nil-macos-aarch64") => "nil"
  end

  test do
    assert_match "nil", shell_output("#{bin}/nil --version")
  end
end